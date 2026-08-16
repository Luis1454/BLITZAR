/*
 * @file engine/src/cuda/fragments/integration/OctreeGpuFinalize.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Finalize a regular GPU Octree update.
 */

/*
 * Module: cuda
 * Responsibility: Apply cosmology, SPH, thermal, and mapped metrics updates.
 */

bool ParticleSystem::finalizeOctreeGpuUpdate(float deltaTime, bool thermalActive,
                                             OctreeGpuUpdateContext& context)
{
    auto& numParticles = context.numParticles;
    auto& adaptiveNumBlocks = context.adaptiveNumBlocks;

    float scaleRatio = 1.0f;
    float previousHubble = 0.0f;
    float nextHubble = 0.0f;
    if (prepareCosmologyStep(deltaTime, scaleRatio, previousHubble, nextHubble)) {
        applyCosmologyExpansionKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
            getSoAView(false), numParticles, scaleRatio, previousHubble, nextHubble);
        if (!checkCudaStatus(cudaGetLastError(), "cosmology expansion kernel launch") ||
            !checkCudaStatus(cudaDeviceSynchronize(), "cosmology expansion kernel sync")) {
            return false;
        }
    }
    if (!this->applySphCorrection(deltaTime, false)) {
        return false;
    }
    _device._hostStateDirty = true;
    if (thermalActive) {
        if (!syncHostState()) {
            return false;
        }
        applyThermalModel(deltaTime);
        syncDeviceState();
    }
    publishMappedMetrics(deltaTime);
    return true;
}
