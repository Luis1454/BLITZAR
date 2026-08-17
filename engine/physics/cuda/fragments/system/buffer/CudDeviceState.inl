/*
 * @file engine/physics/cuda/fragments/system/buffer/CudDeviceState.inl
 * @project BLITZAR
 * @brief Particle-system buffer lifecycle implementation fragment.
 */

bool ParticleSystem::seedDeviceState()
{
    if (_particles.empty())
        return true;
    syncDeviceState();
    return true;
}
