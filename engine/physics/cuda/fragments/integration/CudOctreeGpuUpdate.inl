/*
 * @file engine/physics/cuda/fragments/integration/CudOctreeGpuUpdate.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Prepare and dispatch the GPU Octree and TreePM update pipeline.
 */

/*
 * Module: cuda
 * Responsibility: Select cosmology, graph, or regular GPU Octree execution.
 */

struct ParticleSystem::OctreeGpuUpdateContext final {
    bool profileFlashMode = false;
    bool treePmEnabled = false;
    bool treePmHybrid = false;
    bool treePmPmOnly = false;
    bool treePmLocalGrid = false;
    int treePmMaxLocalNeighbors = 0;
    bool treePmNeighborGrid = false;
    bool treePmGather = false;
    bool treePmMorton = false;
    bool treePmGraphRequested = false;
    ParticleSoAView currentView{};
    ParticleSoAView nextView{};
    int numParticles = 0;
    int adaptiveNumBlocks = 0;
    int rootIndex = -1;
    TreePmGridParams treePmGrid{};
    float treePmCutoffSquared = 0.0f;
};

bool ParticleSystem::updateOctreeGpu(float deltaTime, const ForceLawPolicy& forceLaw,
                                     bool thermalActive)
{
    const bool profileFlashMode = parseBoolEnv("BLITZAR_OCTREE_PROFILE_FLASH", false);
    const bool treePmEnabled = _treePmEnabled && _treePmModel != "exact_tree";
    const bool treePmLegacyModel = _treePmModel == "auto" || _treePmModel.empty();
    const bool treePmHybrid =
        treePmEnabled && _integratorMode == IntegratorMode::Euler && _treePmModel == "hybrid";
    const bool treePmPmOnly = treePmEnabled && _treePmModel == "pm_only";
    const bool treePmLocalGrid =
        treePmEnabled && _integratorMode == IntegratorMode::Euler &&
        (treePmHybrid || treePmPmOnly || (_treePmModel == "local_grid" && _treePmLocalGrid) ||
         (treePmLegacyModel && _treePmLocalGrid));
    const int treePmMaxLocalNeighbors =
        treePmLocalGrid && !treePmPmOnly ? std::clamp(_treePmMaxLocalNeighbors, 0, 256) : 0;
    const bool treePmNeighborGrid =
        treePmLocalGrid && (treePmHybrid || treePmMaxLocalNeighbors > 0);
    bool treePmGather = treePmNeighborGrid && treePmGatherEnabled();
    bool treePmMorton = treePmNeighborGrid && treePmMortonEnabled();
    const bool treePmGraphRequested = parseBoolEnv("BLITZAR_TREEPM_GRAPH", false) && treePmPmOnly &&
                                      !_adaptiveTimeStepsEnabled && !thermalActive &&
                                      !_cosmology.enabled;
    if (_integratorMode == IntegratorMode::Rk4) {
        fprintf(stderr, "[integrator] rk4 is not supported with octree_gpu\n");
        return false;
    }
    if (!_device->d_soaPosX) {
        return false;
    }
    ParticleSoAView currentView = getSoAView(false);
    ParticleSoAView nextView = getSoAView(true);

    const int numParticles = static_cast<int>(_particles.size());
    const int adaptiveNumBlocks =
        (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;

    if (isComovingCosmology(_cosmology)) {
        if (numParticles < 2) {
            return false;
        }
        if (!_device->d_k1v || !_device->d_k2v) {
            if (!allocateRk4Buffers(numParticles)) {
                return false;
            }
        }
        if (!_device->d_vHalf) {
            _device->d_vHalf = static_cast<GpuHalfVelocity*>(bltzr_x::MemoryPool::allocate(
                static_cast<std::size_t>(numParticles) * sizeof(GpuHalfVelocity)));
            if (!_device->d_vHalf) {
                return false;
            }
        }
        const auto hubbleRate = [this](float scaleFactor) {
            const float a = std::max(scaleFactor, 1.0e-6f);
            const float density = _cosmology.omegaRadiation / std::pow(a, 4.0f) +
                                  _cosmology.omegaMatter / std::pow(a, 3.0f) +
                                  _cosmology.omegaLambda;
            return _cosmology.hubbleH0 * std::sqrt(std::max(0.0f, density));
        };
        const float a0 = std::max(_cosmologyScaleFactor, 1.0e-6f);
        const float midpointPredict =
            std::max(a0 + 0.5f * a0 * hubbleRate(a0) * deltaTime, 1.0e-6f);
        const float a1 =
            std::max(a0, a0 + midpointPredict * hubbleRate(midpointPredict) * deltaTime);
        const float amid = 0.5f * (a0 + a1);
        const auto driftIntegrand = [&hubbleRate](float a) {
            return 1.0f / (a * a * a * std::max(hubbleRate(a), 1.0e-12f));
        };
        const float drift =
            (a1 - a0) * (driftIntegrand(a0) + 4.0f * driftIntegrand(amid) + driftIntegrand(a1)) /
            6.0f;
        if (a1 <= a0 || drift <= 0.0f) {
            return false;
        }
        TreePmGridParams grid{};
        float unusedCutoff = 0.0f;
        _cosmologyScaleFactor = amid;
        if (!buildTreePmGrid(currentView, numParticles, &grid, &unusedCutoff)) {
            return false;
        }
        computeTreePmPmOnlyAccelerationKernel<<<adaptiveNumBlocks,
                                                Particle::kDefaultCudaBlockSize>>>(
            currentView, _device->d_k1v, numParticles, grid, _device->d_treePmAccelX,
            _device->d_treePmAccelY, _device->d_treePmAccelZ);
        auto* momentumHalf = reinterpret_cast<float3*>(_device->d_vHalf.get());
        applyKickHalfStepKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device->d_k1v, deltaTime, momentumHalf, numParticles);
        cosmologyDriftKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, momentumHalf, drift, 2.0f * _cosmology.boxHalfExtent, nextView,
            numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "cosmology first KDK launch") ||
            !checkCudaStatus(cudaDeviceSynchronize(), "cosmology first KDK sync")) {
            return false;
        }
        std::swap(_device->d_soaPosX, _device->d_soaNextPosX);
        std::swap(_device->d_soaPosY, _device->d_soaNextPosY);
        std::swap(_device->d_soaPosZ, _device->d_soaNextPosZ);
        std::swap(_device->d_soaVelX, _device->d_soaNextVelX);
        std::swap(_device->d_soaVelY, _device->d_soaNextVelY);
        std::swap(_device->d_soaVelZ, _device->d_soaNextVelZ);
        currentView = getSoAView(false);
        nextView = getSoAView(true);
        _cosmologyScaleFactor = a1;
        if (!buildTreePmGrid(currentView, numParticles, &grid, &unusedCutoff)) {
            return false;
        }
        computeTreePmPmOnlyAccelerationKernel<<<adaptiveNumBlocks,
                                                Particle::kDefaultCudaBlockSize>>>(
            currentView, _device->d_k2v, numParticles, grid, _device->d_treePmAccelX,
            _device->d_treePmAccelY, _device->d_treePmAccelZ);
        finalizeLeapfrogKickKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, momentumHalf, _device->d_k2v, deltaTime, nextView, momentumHalf,
            numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "cosmology final KDK launch") ||
            !checkCudaStatus(cudaDeviceSynchronize(), "cosmology final KDK sync")) {
            return false;
        }
        std::swap(_device->d_soaPosX, _device->d_soaNextPosX);
        std::swap(_device->d_soaPosY, _device->d_soaNextPosY);
        std::swap(_device->d_soaPosZ, _device->d_soaNextPosZ);
        std::swap(_device->d_soaVelX, _device->d_soaNextVelX);
        std::swap(_device->d_soaVelY, _device->d_soaNextVelY);
        std::swap(_device->d_soaVelZ, _device->d_soaNextVelZ);
        _cosmologyTime += deltaTime;
        _device->_hostStateDirty = true;
        if (!_cosmologyMarkerPrinted) {
            fprintf(stderr,
                    "[cosmology] mode=comoving backend=cuda_pm assignment=tsc box=%.6g a0=%.6g\n",
                    2.0f * _cosmology.boxHalfExtent, a0);
            _cosmologyMarkerPrinted = true;
        }

        publishMappedMetrics(deltaTime);
        return true;
    }

    if (treePmGraphRequested && _device->_treePmGraphCaptured[_device->_treePmGraphSlot]) {
        if (!launchTreePmGraph(_device->_treePmGraphSlot)) {
            return false;
        }
        std::swap(_device->d_soaPosX, _device->d_soaNextPosX);
        std::swap(_device->d_soaPosY, _device->d_soaNextPosY);
        std::swap(_device->d_soaPosZ, _device->d_soaNextPosZ);
        std::swap(_device->d_soaVelX, _device->d_soaNextVelX);
        std::swap(_device->d_soaVelY, _device->d_soaNextVelY);
        std::swap(_device->d_soaVelZ, _device->d_soaNextVelZ);
        _device->_treePmGraphSlot ^= 1;
        _device->_hostStateDirty = true;
        if (!_device->_treePmGraphMarkerPrinted) {
            fprintf(stderr, "[treepm] cuda_graph=active model=pm_only static_mesh_pipeline=1\n");
            _device->_treePmGraphMarkerPrinted = true;
        }
        publishMappedMetrics(deltaTime);
        return true;
    }
    OctreeGpuUpdateContext context{};
    context.profileFlashMode = profileFlashMode;
    context.treePmEnabled = treePmEnabled;
    context.treePmHybrid = treePmHybrid;
    context.treePmPmOnly = treePmPmOnly;
    context.treePmLocalGrid = treePmLocalGrid;
    context.treePmMaxLocalNeighbors = treePmMaxLocalNeighbors;
    context.treePmNeighborGrid = treePmNeighborGrid;
    context.treePmGather = treePmGather;
    context.treePmMorton = treePmMorton;
    context.treePmGraphRequested = treePmGraphRequested;
    context.currentView = currentView;
    context.nextView = nextView;
    context.numParticles = numParticles;
    context.adaptiveNumBlocks = adaptiveNumBlocks;

    return updateOctreeGpuRegular(deltaTime, forceLaw, thermalActive, context);
}
