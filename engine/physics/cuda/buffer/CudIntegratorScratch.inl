/*
 * @file engine/physics/cuda/buffer/CudIntegratorScratch.inl
 * @project BLITZAR
 * @brief Particle-system buffer lifecycle implementation fragment.
 */

bool ParticleSystem::allocateRk4Buffers(int numParticles)
{
    if (!_device->_cudaRuntimeAvailable) {
        return false;
    }
    releaseRk4Buffers();
    _device->d_stage =
        static_cast<Particle*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Particle)));
    _device->d_k1x =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device->d_k2x =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device->d_k3x =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device->d_k4x =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device->d_k1v =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device->d_k2v =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device->d_k3v =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    _device->d_k4v =
        static_cast<Vector3*>(bltzr_x::MemoryPool::allocate(numParticles * sizeof(Vector3)));
    if (_integratorMode == IntegratorMode::Leapfrog) {
        _device->d_vHalf = static_cast<GpuHalfVelocity*>(
            bltzr_x::MemoryPool::allocate(numParticles * sizeof(GpuHalfVelocity)));
    }

    if (!_device->d_stage || !_device->d_k1x || !_device->d_k2x || !_device->d_k3x || !_device->d_k4x ||
        !_device->d_k1v || !_device->d_k2v || !_device->d_k3v || !_device->d_k4v ||
        (_integratorMode == IntegratorMode::Leapfrog && !_device->d_vHalf)) {
        releaseRk4Buffers();
        return false;
    }
    return true;
}

/*
 * @brief Documents the release rk4 buffers operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ParticleSystem::releaseRk4Buffers()
{
    if (_device->d_stage) {
        bltzr_x::MemoryPool::deallocate(_device->d_stage);
        _device->d_stage = nullptr;
    }
    if (_device->d_k1x) {
        bltzr_x::MemoryPool::deallocate(_device->d_k1x);
        _device->d_k1x = nullptr;
    }
    if (_device->d_k2x) {
        bltzr_x::MemoryPool::deallocate(_device->d_k2x);
        _device->d_k2x = nullptr;
    }
    if (_device->d_k3x) {
        bltzr_x::MemoryPool::deallocate(_device->d_k3x);
        _device->d_k3x = nullptr;
    }
    if (_device->d_k4x) {
        bltzr_x::MemoryPool::deallocate(_device->d_k4x);
        _device->d_k4x = nullptr;
    }
    if (_device->d_k1v) {
        bltzr_x::MemoryPool::deallocate(_device->d_k1v);
        _device->d_k1v = nullptr;
    }
    if (_device->d_k2v) {
        bltzr_x::MemoryPool::deallocate(_device->d_k2v);
        _device->d_k2v = nullptr;
    }
    if (_device->d_k3v) {
        bltzr_x::MemoryPool::deallocate(_device->d_k3v);
        _device->d_k3v = nullptr;
    }
    if (_device->d_k4v) {
        bltzr_x::MemoryPool::deallocate(_device->d_k4v);
        _device->d_k4v = nullptr;
    }
    if (_device->d_vHalf) {
        bltzr_x::MemoryPool::deallocate(_device->d_vHalf);
        _device->d_vHalf = nullptr;
    }
}

/*
 * @brief Documents the allocate sph buffers operation contract.
 * @param numParticles Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
