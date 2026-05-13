/*
 * @file engine/src/cuda/fragments/system/State.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: cuda
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
    if (!_device._cudaRuntimeAvailable) {
        return;
    }
    if (_particles.empty())
        return;
    const int numParticles = static_cast<int>(_particles.size());

    // Avoid an intermediate AoS->SoA pack kernel by creating host-side SoA buffers
    // and copying them directly to the device SoA arrays. This saves a global
    // kernel launch and reduces host->device memory traffic.
    if (!_device.d_soaPosX) {
        if (!allocateParticleBuffers(static_cast<std::size_t>(numParticles)))
            return;
    }

    const int uploadChunkMax = std::max(1, numParticles);
    const int uploadChunkMin = std::min(65536, uploadChunkMax);
    const int uploadChunkParticles = std::clamp(
        static_cast<int>(parseFloatEnv("BLITZAR_DEVICE_UPLOAD_CHUNK_PARTICLES", 4194304.0f)),
        uploadChunkMin, uploadChunkMax);
    const std::size_t chunkCapacity = static_cast<std::size_t>(uploadChunkParticles);
    std::vector<float> hostPosX(chunkCapacity);
    std::vector<float> hostPosY(chunkCapacity);
    std::vector<float> hostPosZ(chunkCapacity);
    std::vector<float> hostMass(chunkCapacity);

    for (std::size_t offset = 0u; offset < _particles.size(); offset += chunkCapacity) {
        const std::size_t chunkCount = std::min(chunkCapacity, _particles.size() - offset);
        const int chunkCountInt = static_cast<int>(chunkCount);

#pragma omp parallel for schedule(static)
        for (int localIndex = 0; localIndex < chunkCountInt; ++localIndex) {
            const std::size_t particleIndex = offset + static_cast<std::size_t>(localIndex);
            const Vector3 p = _particles[particleIndex].getPosition();
            const std::size_t hostIndex = static_cast<std::size_t>(localIndex);
            hostPosX[hostIndex] = p.x;
            hostPosY[hostIndex] = p.y;
            hostPosZ[hostIndex] = p.z;
            hostMass[hostIndex] = _particles[particleIndex].getMass();
        }

        const std::size_t bytesFloats = chunkCount * sizeof(float);
        if (!checkCudaStatus(cudaMemcpy(_device.d_soaPosX + offset, hostPosX.data(), bytesFloats,
                                        cudaMemcpyHostToDevice),
                             "memcpy(HtoD soa posX)"))
            return;
        if (!checkCudaStatus(cudaMemcpy(_device.d_soaPosY + offset, hostPosY.data(), bytesFloats,
                                        cudaMemcpyHostToDevice),
                             "memcpy(HtoD soa posY)"))
            return;
        if (!checkCudaStatus(cudaMemcpy(_device.d_soaPosZ + offset, hostPosZ.data(), bytesFloats,
                                        cudaMemcpyHostToDevice),
                             "memcpy(HtoD soa posZ)"))
            return;
        if (!checkCudaStatus(cudaMemcpy(_device.d_soaMass + offset, hostMass.data(), bytesFloats,
                                        cudaMemcpyHostToDevice),
                             "memcpy(HtoD soa mass)"))
            return;
    }

    _device._hostStateDirty = false;
    _device._leapfrogPrimed = false;
}

/*
 * @brief Documents the sync host state operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ParticleSystem::syncHostState()
{
    if (!_device._cudaRuntimeAvailable) {
        return !_device._hostStateDirty;
    }
    if (!_device._hostStateDirty)
        return true;
    const int numParticles = static_cast<int>(_particles.size());
    const std::size_t bytes = _particles.size() * sizeof(Particle);

    if (!_device.d_stage) {
        if (!allocateRk4Buffers(numParticles))
            return false;
    }

    ParticleSoAView view = getSoAView(false);
    const int numBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    unpackSoAKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(view, _device.d_stage, numParticles);
    cudaDeviceSynchronize();

    if (!checkCudaStatus(cudaMemcpy(_particles.data(), _device.d_stage, bytes, cudaMemcpyDeviceToHost),
                         "memcpy(DtoH stage)"))
        return false;

    _device._hostStateDirty = false;
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
        view.posX = _device.d_soaNextPosX;
        view.posY = _device.d_soaNextPosY;
        view.posZ = _device.d_soaNextPosZ;
        view.velX = _device.d_soaNextVelX;
        view.velY = _device.d_soaNextVelY;
        view.velZ = _device.d_soaNextVelZ;
    }
    else {
        view.posX = _device.d_soaPosX;
        view.posY = _device.d_soaPosY;
        view.posZ = _device.d_soaPosZ;
        view.velX = _device.d_soaVelX;
        view.velY = _device.d_soaVelY;
        view.velZ = _device.d_soaVelZ;
    }
    view.pressX = _device.d_soaPressX;
    view.pressY = _device.d_soaPressY;
    view.pressZ = _device.d_soaPressZ;
    view.mass = _device.d_soaMass;
    view.temp = _device.d_soaTemp;
    view.dens = _device.d_soaDens;
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
    constexpr std::uint32_t kMetricsPublishStride = 8u;
    if (!_device._cudaRuntimeAvailable) {
        return;
    }
    if (_device._mappedMetricsDevice == nullptr || _device.d_soaPosX == nullptr || _particles.empty()) {
        return;
    }

    _device._metricsStepId += 1u;
    _device._metricsSimTime += deltaTime;
    _device._metricsPublishCounter += 1u;
    if ((_device._metricsPublishCounter % kMetricsPublishStride) != 0u) {
        return;
    }

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
    publishMetricsKernel<<<1, 1>>>(_device._mappedMetricsDevice, currentView,
                                   static_cast<int>(_particles.size()), _device._metricsStepId,
                                   _device._metricsSimTime, deltaTime, vramUsedBytes, vramPeakBytes);
    checkCudaStatus(cudaGetLastError(), "publishMetricsKernel launch");
}
