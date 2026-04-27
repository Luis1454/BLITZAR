/*
 * @file engine/src/physics/cuda/fragments/ParticleSystemState.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: physics/cuda
 * Responsibility: Synchronize particle-system state between host and device views.
 */

/*
 * @brief Documents the sync device state operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::syncDeviceState()
{
    if (!_cudaRuntimeAvailable) {
        return;
    }
    if (_particles.empty())
        return;
    const int numParticles = static_cast<int>(_particles.size());
    const std::size_t bytes = _particles.size() * sizeof(Particle);

    if (!d_stage) {
        if (!allocateRk4Buffers(numParticles))
            return;
    }

    if (!checkCudaStatus(cudaMemcpy(d_stage, _particles.data(), bytes, cudaMemcpyHostToDevice),
                         "memcpy(HtoD stage)"))
        return;

    ParticleSoAView view = getSoAView(false);
    const int numBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    packSoAKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(d_stage, view, numParticles);
    cudaDeviceSynchronize();
    _hostStateDirty = false;
    _leapfrogPrimed = false;
}

/*
 * @brief Documents the sync host state operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::syncHostState()
{
    if (!_cudaRuntimeAvailable) {
        return !_hostStateDirty;
    }
    if (!_hostStateDirty)
        return true;
    const int numParticles = static_cast<int>(_particles.size());
    const std::size_t bytes = _particles.size() * sizeof(Particle);

    if (!d_stage) {
        if (!allocateRk4Buffers(numParticles))
            return false;
    }

    ParticleSoAView view = getSoAView(false);
    const int numBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    unpackSoAKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(view, d_stage, numParticles);
    cudaDeviceSynchronize();

    if (!checkCudaStatus(cudaMemcpy(_particles.data(), d_stage, bytes, cudaMemcpyDeviceToHost),
                         "memcpy(DtoH stage)"))
        return false;

    _hostStateDirty = false;
    return true;
}

/*
 * @brief Documents the get so aview operation contract.
 * @param next Input value used by this contract.
 * @return ParticleSoAView ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ParticleSoAView ParticleSystem::getSoAView(bool next) const
{
    ParticleSoAView view;
    if (next) {
        view.posX = d_soaNextPosX;
        view.posY = d_soaNextPosY;
        view.posZ = d_soaNextPosZ;
        view.velX = d_soaNextVelX;
        view.velY = d_soaNextVelY;
        view.velZ = d_soaNextVelZ;
    }
    else {
        view.posX = d_soaPosX;
        view.posY = d_soaPosY;
        view.posZ = d_soaPosZ;
        view.velX = d_soaVelX;
        view.velY = d_soaVelY;
        view.velZ = d_soaVelZ;
    }
    view.pressX = d_soaPressX;
    view.pressY = d_soaPressY;
    view.pressZ = d_soaPressZ;
    view.mass = d_soaMass;
    view.temp = d_soaTemp;
    view.dens = d_soaDens;
    view.count = static_cast<int>(_particles.size());
    return view;
}

/*
 * @brief Documents the publish mapped metrics operation contract.
 * @param deltaTime Input value used by this contract.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::publishMappedMetrics(float deltaTime)
{
    if (!_cudaRuntimeAvailable) {
        return;
    }
    if (_mappedMetricsDevice == nullptr || d_soaPosX == nullptr || _particles.empty()) {
        return;
    }

    _metricsStepId += 1u;
    _metricsSimTime += deltaTime;

    std::uint64_t vramUsedBytes = 0u;
    std::uint64_t vramPeakBytes = 0u;
    int device = 0;
    cudaMemPool_t pool = nullptr;
    if (cudaGetDevice(&device) == cudaSuccess &&
        cudaDeviceGetDefaultMemPool(&pool, device) == cudaSuccess && pool != nullptr) {
        cudaMemPoolGetAttribute(pool, cudaMemPoolAttrUsedMemCurrent, &vramUsedBytes);
        cudaMemPoolGetAttribute(pool, cudaMemPoolAttrUsedMemHigh, &vramPeakBytes);
    }

    const ParticleSoAView currentView = getSoAView(false);
    publishMetricsKernel<<<1, 1>>>(_mappedMetricsDevice, currentView,
                                   static_cast<int>(_particles.size()), _metricsStepId,
                                   _metricsSimTime, deltaTime, vramUsedBytes, vramPeakBytes);
    checkCudaStatus(cudaGetLastError(), "publishMetricsKernel launch");
}
