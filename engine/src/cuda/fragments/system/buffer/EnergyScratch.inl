/*
 * @file engine/src/cuda/fragments/system/buffer/EnergyScratch.inl
 * @project BLITZAR
 * @brief Particle-system buffer lifecycle implementation fragment.
 */

bool ParticleSystem::ensureEnergyScratchCapacity(int numParticles, int sampleCount)
{
    if (!_device._cudaRuntimeAvailable) {
        return false;
    }
    if (numParticles <= 0 || sampleCount <= 0) {
        return false;
    }
    const int blockCount =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    if (_device.d_energyBlockCapacity < static_cast<std::size_t>(blockCount)) {
        if (_device.d_energyKineticBlocks) {
            bltzr_x::MemoryPool::deallocate(_device.d_energyKineticBlocks);
            _device.d_energyKineticBlocks = nullptr;
        }
        if (_device.d_energyThermalBlocks) {
            bltzr_x::MemoryPool::deallocate(_device.d_energyThermalBlocks);
            _device.d_energyThermalBlocks = nullptr;
        }
        _device.d_energyKineticBlocks = static_cast<float*>(
            bltzr_x::MemoryPool::allocate(static_cast<std::size_t>(blockCount) * sizeof(float)));
        _device.d_energyThermalBlocks = static_cast<float*>(
            bltzr_x::MemoryPool::allocate(static_cast<std::size_t>(blockCount) * sizeof(float)));
        if (!_device.d_energyKineticBlocks || !_device.d_energyThermalBlocks) {
            if (_device.d_energyKineticBlocks) {
                bltzr_x::MemoryPool::deallocate(_device.d_energyKineticBlocks);
                _device.d_energyKineticBlocks = nullptr;
            }
            if (_device.d_energyThermalBlocks) {
                bltzr_x::MemoryPool::deallocate(_device.d_energyThermalBlocks);
                _device.d_energyThermalBlocks = nullptr;
            }
            _device.d_energyBlockCapacity = 0;
            return false;
        }
        _device.d_energyBlockCapacity = static_cast<std::size_t>(blockCount);
    }

    if (_device.d_energySampleCapacity < static_cast<std::size_t>(sampleCount)) {
        if (_device.d_energyPotentialPartials) {
            bltzr_x::MemoryPool::deallocate(_device.d_energyPotentialPartials);
            _device.d_energyPotentialPartials = nullptr;
        }
        _device.d_energyPotentialPartials = static_cast<double*>(
            bltzr_x::MemoryPool::allocate(static_cast<std::size_t>(sampleCount) * sizeof(double)));
        if (!_device.d_energyPotentialPartials) {
            _device.d_energySampleCapacity = 0;
            return false;
        }
        _device.d_energySampleCapacity = static_cast<std::size_t>(sampleCount);
    }

    return true;
}

/*
 * @brief Documents the allocate rk4 buffers operation contract.
 * @param numParticles Input value used by this contract.
 * @return bool ParticleSystem:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
