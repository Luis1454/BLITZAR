/*
 * @file engine/src/cuda/fragments/system/buffer/ParticleBuffers.inl
 * @project BLITZAR
 * @brief Particle-system buffer lifecycle implementation fragment.
 */

bool ParticleSystem::allocateParticleBuffers(std::size_t particleCapacity)
{
    if (!_device->_cudaRuntimeAvailable) {
        return false;
    }
    const std::size_t bytesTotal = particleCapacity * sizeof(float);
    const bool gravityOnlyBuffers =
        treePmUsesGravityOnlyBuffers(_integratorMode == IntegratorMode::Euler, _sphEnabled);
    bool ok = true;
    ok &= ((_device->d_soaPosX = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device->d_soaPosY = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device->d_soaPosZ = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device->d_soaVelX = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device->d_soaVelY = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device->d_soaVelZ = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device->d_soaMass = static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) !=
           nullptr);
    ok &= ((_device->d_soaPressX =
                static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((_device->d_soaPressY =
                static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((_device->d_soaPressZ =
                static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
    if (!gravityOnlyBuffers) {
        ok &= ((_device->d_soaTemp =
                    static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
        ok &= ((_device->d_soaDens =
                    static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
    }
    ok &= ((_device->d_soaNextPosX =
                static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((_device->d_soaNextPosY =
                static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((_device->d_soaNextPosZ =
                static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((_device->d_soaNextVelX =
                static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((_device->d_soaNextVelY =
                static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);
    ok &= ((_device->d_soaNextVelZ =
                static_cast<float*>(bltzr_x::MemoryPool::allocate(bytesTotal))) != nullptr);

    if (!ok) {
        releaseParticleBuffers();
        return false;
    }
    return true;
}
