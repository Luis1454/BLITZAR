/*
 * @file engine/src/cuda/fragments/treepm/Layout.inl
 * @project BLITZAR
 * @brief TreePM layout selection and particle-system layout state.
 */

// The ParticleSystem member definitions below remain in the owning global namespace. A namespace
// alias keeps every cross-namespace CUDA helper reference explicit without importing symbols.
namespace treepm = blitzar_cuda_tree_pm_gpu;

int ParticleSystem::treePmLayoutMode()
{
    if (_device._treePmLayoutModeInitialized) {
        if (_device._treePmLayoutMode != treepm::kTreePmLayoutAuto) {
            return _device._treePmLayoutMode;
        }
        return _device._treePmAutoLayoutResolved
                   ? (_device._treePmAutoGather
                          ? (_device._treePmAutoMorton ? treepm::kTreePmLayoutGatherMorton
                                                       : treepm::kTreePmLayoutGatherLinear)
                          : treepm::kTreePmLayoutLinear)
                   : treepm::kTreePmLayoutAuto;
    }

    _device._treePmLayoutModeInitialized = true;
    _device._treePmLayoutMode = treepm::kTreePmLayoutLegacy;
    const auto environmentLayout = bltzr_env::get("BLITZAR_TREEPM_LAYOUT");
    const std::string configured = environmentLayout.has_value() ? *environmentLayout : _treePmLayout;
    if (configured.empty()) {
        const bool legacyFlags = bltzr_env::get("BLITZAR_TREEPM_GATHER").has_value() ||
                                 bltzr_env::get("BLITZAR_TREEPM_MORTON").has_value();
        _device._treePmLayoutMode = legacyFlags ? treepm::kTreePmLayoutLegacy : treepm::kTreePmLayoutAuto;
        return _device._treePmLayoutMode;
    }
    if (configured == "auto") {
        _device._treePmLayoutMode = treepm::kTreePmLayoutAuto;
        return treepm::kTreePmLayoutAuto;
    }
    if (configured == "linear") {
        _device._treePmLayoutMode = treepm::kTreePmLayoutLinear;
        return treepm::kTreePmLayoutLinear;
    }
    if (configured == "gather_linear") {
        _device._treePmLayoutMode = treepm::kTreePmLayoutGatherLinear;
        return treepm::kTreePmLayoutGatherLinear;
    }
    if (configured == "gather_morton") {
        _device._treePmLayoutMode = treepm::kTreePmLayoutGatherMorton;
        return treepm::kTreePmLayoutGatherMorton;
    }
    fprintf(stderr, "[treepm] invalid layout=%s fallback=auto\n", configured.c_str());
    _device._treePmLayoutMode = treepm::kTreePmLayoutAuto;
    return treepm::kTreePmLayoutAuto;
}
__global__ void computeTreePmPmOnlyAccelerationKernel(ParticleSoAView state,
                                                       Vector3Handle outAcceleration,
                                                       int numParticles,
                                                       TreePmGridParams grid,
                                                       const float* accelX,
                                                       const float* accelY,
                                                       const float* accelZ)
{
    const int particleIndex = blockIdx.x * blockDim.x + threadIdx.x;
    if (particleIndex < numParticles) {
        outAcceleration[particleIndex] = treepm::treePmSampleAcceleration(
            grid, octreeLoadParticlePosition(state, particleIndex), accelX, accelY, accelZ);
    }
}

bool ParticleSystem::treePmGatherEnabled()
{
    const int layout = treePmLayoutMode();
    if (layout == treepm::kTreePmLayoutAuto) {
        return _device._treePmAutoLayoutResolved && _device._treePmAutoGather;
    }
    if (layout == treepm::kTreePmLayoutGatherLinear || layout == treepm::kTreePmLayoutGatherMorton) {
        return true;
    }
    if (layout == treepm::kTreePmLayoutLinear) {
        return false;
    }
    return parseBoolEnv("BLITZAR_TREEPM_GATHER", false);
}

bool ParticleSystem::treePmMortonEnabled()
{
    const int layout = treePmLayoutMode();
    if (layout == treepm::kTreePmLayoutAuto) {
        return _device._treePmAutoLayoutResolved && _device._treePmAutoMorton;
    }
    if (layout == treepm::kTreePmLayoutGatherMorton) {
        return true;
    }
    if (layout == treepm::kTreePmLayoutLinear || layout == treepm::kTreePmLayoutGatherLinear) {
        return false;
    }
    return parseBoolEnv("BLITZAR_TREEPM_MORTON", false);
}
