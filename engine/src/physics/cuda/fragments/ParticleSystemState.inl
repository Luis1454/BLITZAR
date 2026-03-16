void ParticleSystem::syncDeviceState()
{
    if (_particles.empty()) return;
    const int numParticles = static_cast<int>(_particles.size());
    const std::size_t bytes = _particles.size() * sizeof(Particle);

    if (!d_stage) {
        if (!allocateRk4Buffers(numParticles)) return;
    }

    if (!checkCudaStatus(cudaMemcpy(d_stage, _particles.data(), bytes, cudaMemcpyHostToDevice), "memcpy(HtoD stage)")) return;

    ParticleSoAView view = getSoAView(false);
    const int numBlocks = (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    packSoAKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(d_stage, view, numParticles);
    cudaDeviceSynchronize();
    _hostStateDirty = false;
}

bool ParticleSystem::syncHostState()
{
    if (!_hostStateDirty) return true;
    const int numParticles = static_cast<int>(_particles.size());
    const std::size_t bytes = _particles.size() * sizeof(Particle);

    if (!d_stage) {
        if (!allocateRk4Buffers(numParticles)) return false;
    }

    ParticleSoAView view = getSoAView(false);
    const int numBlocks = (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    unpackSoAKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(view, d_stage, numParticles);
    cudaDeviceSynchronize();

    if (!checkCudaStatus(cudaMemcpy(_particles.data(), d_stage, bytes, cudaMemcpyDeviceToHost), "memcpy(DtoH stage)")) return false;

    _hostStateDirty = false;
    return true;
}

ParticleSoAView ParticleSystem::getSoAView(bool next) const
{
    ParticleSoAView view;
    if (next) {
        view.posX = d_soaNextPosX; view.posY = d_soaNextPosY; view.posZ = d_soaNextPosZ;
        view.velX = d_soaNextVelX; view.velY = d_soaNextVelY; view.velZ = d_soaNextVelZ;
    } else {
        view.posX = d_soaPosX; view.posY = d_soaPosY; view.posZ = d_soaPosZ;
        view.velX = d_soaVelX; view.velY = d_soaVelY; view.velZ = d_soaVelZ;
    }
    view.pressX = d_soaPressX; view.pressY = d_soaPressY; view.pressZ = d_soaPressZ;
    view.mass = d_soaMass; view.temp = d_soaTemp; view.dens = d_soaDens;
    view.count = static_cast<int>(_particles.size());
    return view;
}
