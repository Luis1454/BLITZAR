/*
 * @file engine/src/cuda/fragments/system/buffer/AdaptiveScratch.inl
 * @project BLITZAR
 * @brief Particle-system buffer lifecycle implementation fragment.
 */

bool ParticleSystem::ensureAdaptiveCudaScratchCapacity(int numParticles)
{
    if (!_device._cudaRuntimeAvailable || numParticles <= 0) {
        return false;
    }
    const std::size_t count = static_cast<std::size_t>(numParticles);
    if (_device.d_adaptiveCapacity >= count && _device.d_adaptiveAcceleration != nullptr &&
        _device.d_adaptiveLevels != nullptr && _device.d_adaptiveLastForceTicks != nullptr) {
        return true;
    }

    bltzr_x::MemoryPool::deallocate(_device.d_adaptiveAcceleration);
    bltzr_x::MemoryPool::deallocate(_device.d_adaptiveLevels);
    bltzr_x::MemoryPool::deallocate(_device.d_adaptiveLastForceTicks);
    _device.d_adaptiveAcceleration = static_cast<Vector3*>(
        bltzr_x::MemoryPool::allocate(count * sizeof(Vector3)));
    _device.d_adaptiveLevels = static_cast<std::uint8_t*>(
        bltzr_x::MemoryPool::allocate(count * sizeof(std::uint8_t)));
    _device.d_adaptiveLastForceTicks = static_cast<std::uint64_t*>(
        bltzr_x::MemoryPool::allocate(count * sizeof(std::uint64_t)));
    if (_device.d_adaptiveAcceleration == nullptr || _device.d_adaptiveLevels == nullptr ||
        _device.d_adaptiveLastForceTicks == nullptr) {
        bltzr_x::MemoryPool::deallocate(_device.d_adaptiveAcceleration);
        bltzr_x::MemoryPool::deallocate(_device.d_adaptiveLevels);
        bltzr_x::MemoryPool::deallocate(_device.d_adaptiveLastForceTicks);
        _device.d_adaptiveAcceleration = nullptr;
        _device.d_adaptiveLevels = nullptr;
        _device.d_adaptiveLastForceTicks = nullptr;
        _device.d_adaptiveCapacity = 0u;
        return false;
    }
    _device.d_adaptiveCapacity = count;
    return true;
}

/*
 * @brief Documents the release particle buffers operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
