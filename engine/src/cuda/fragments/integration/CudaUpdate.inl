/*
 * @file engine/src/cuda/fragments/integration/CudaUpdate.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Dispatch generic CUDA pairwise and fallback solver updates.
 */

/*
 * Module: cuda
 * Responsibility: Select the adaptive or standard generic CUDA integration path.
 */

bool ParticleSystem::updateCudaSolvers(float deltaTime, const ForceLawPolicy& forceLaw,
                                       bool thermalActive)
{
    if (!_device._cudaRuntimeAvailable || !_device.d_soaPosX) {
        return false;
    }

    const int numParticles = static_cast<int>(_particles.size());
    const int numBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
    ParticleSoAView currentView = getSoAView(false);
    ParticleSoAView nextView = getSoAView(true);

    if (_adaptiveTimeStepsEnabled && !_adaptiveTimeStepCostGuard &&
        _solverMode == SolverMode::PairwiseCuda) {
        return updateCudaAdaptive(deltaTime, forceLaw, thermalActive, currentView, nextView,
                                  numParticles, numBlocks);
    }

    return updateCudaIntegrators(deltaTime, forceLaw, thermalActive, currentView, nextView,
                                 numParticles, numBlocks);
}
