/*
 * @file engine/physics/cuda/fragments/integration/OctreeGpuAdaptive.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Advance the GPU Octree with dyadic adaptive time steps.
 */

/*
 * Module: cuda
 * Responsibility: Execute the native adaptive Octree scheduler.
 */

bool ParticleSystem::updateOctreeGpuAdaptive(float deltaTime, const ForceLawPolicy& forceLaw,
                                             bool thermalActive, OctreeGpuUpdateContext& context)
{
    auto& currentView = context.currentView;
    auto& nextView = context.nextView;
    auto& numParticles = context.numParticles;
    auto& adaptiveNumBlocks = context.adaptiveNumBlocks;
    auto& treePmEnabled = context.treePmEnabled;
    auto& treePmHybrid = context.treePmHybrid;
    auto& treePmLocalGrid = context.treePmLocalGrid;
    auto& treePmMaxLocalNeighbors = context.treePmMaxLocalNeighbors;
    auto& treePmGather = context.treePmGather;
    auto& rootIndex = context.rootIndex;
    auto& treePmGrid = context.treePmGrid;
    auto& treePmCutoffSquared = context.treePmCutoffSquared;

    if (_adaptiveTimeStepsEnabled && !_adaptiveTimeStepCostGuard) {
        const std::uint32_t levelCount = std::min<std::uint32_t>(_adaptiveTimeStepMaxLevel, 12u);
        const std::uint32_t sliceCount = 1u << levelCount;
        const float quantum = deltaTime / static_cast<float>(sliceCount);
        if (quantum <= 0.0f || !ensureAdaptiveCudaScratchCapacity(numParticles)) {
            fprintf(stderr, "[adaptive] CUDA scratch allocation failed\n");
            return false;
        }

        AdaptiveGpuForceContext forceContext{};
        forceContext.mode = treePmHybrid ? 3 : treePmLocalGrid ? 1 : treePmEnabled ? 2 : 0;
        forceContext.nodeHot = _device->d_octreeNodeHot;
        forceContext.nodeNav = _device->d_octreeNodeNav;
        forceContext.nodeFirstChild = _device->d_octreeFirstChild;
        forceContext.leafStarts = _device->d_octreeLeafStarts;
        forceContext.leafCounts = _device->d_octreeLeafCounts;
        forceContext.rootIndex = rootIndex;
        forceContext.leafIndices = _device->g_dOctreeLeafIndices;
        forceContext.forceLaw = forceLaw;
        forceContext.maxAcceleration = _physicsMaxAcceleration;
        forceContext.openingCriterion =
            _octreeOpeningCriterion == OctreeOpeningCriterion::Bounds ? 1 : 0;
        forceContext.grid = treePmGrid;
        forceContext.sortedIndex = _device->d_sphSortedIndex;
        forceContext.cellStart = _device->d_sphCellStart;
        forceContext.cellEnd = _device->d_sphCellEnd;
        forceContext.pmAccelX = _device->d_treePmAccelX;
        forceContext.pmAccelY = _device->d_treePmAccelY;
        forceContext.pmAccelZ = _device->d_treePmAccelZ;
        forceContext.cellMask = _device->d_treePmCellMask;
        forceContext.cutoffSquared = treePmCutoffSquared;
        forceContext.cellRadius = std::clamp(
            static_cast<int>(std::ceil(std::sqrt(treePmCutoffSquared) * treePmGrid.invCellSize)), 1,
            2);
        forceContext.maxLocalNeighbors = treePmMaxLocalNeighbors;
        forceContext.sortedPosX = treePmGather ? _device->d_treePmSortedPosX.get() : nullptr;
        forceContext.sortedPosY = treePmGather ? _device->d_treePmSortedPosY.get() : nullptr;
        forceContext.sortedPosZ = treePmGather ? _device->d_treePmSortedPosZ.get() : nullptr;
        forceContext.sortedMass = treePmGather ? _device->d_treePmSortedMass.get() : nullptr;
        forceContext.denseCellThreshold = std::max(_treePmDenseCellThreshold, 1);

        const bool resetSchedule =
            _adaptiveTimeStepTick == 0u || std::abs(_adaptiveTimeStepQuantum - quantum) > 1.0e-12f;
        if (resetSchedule) {
            computeAdaptiveForceKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, _device->d_adaptiveAcceleration, numParticles, forceContext);
            if (!checkCudaStatus(cudaGetLastError(), "adaptive octree initial force launch")) {
                return false;
            }
            initializeAdaptiveScheduleKernel<<<adaptiveNumBlocks,
                                               Particle::kDefaultCudaBlockSize>>>(
                currentView, _device->d_adaptiveAcceleration, _device->d_adaptiveLevels,
                _device->d_adaptiveLastForceTicks, numParticles, static_cast<int>(levelCount),
                _adaptiveTimeStepEta, std::max(_octreeSoftening, _physicsMinSoftening), deltaTime);
            if (!checkCudaStatus(cudaGetLastError(), "adaptive octree schedule launch")) {
                return false;
            }
            _adaptiveTimeStepQuantum = quantum;
        }

        for (std::uint32_t slice = 0u; slice < sliceCount; ++slice) {
            const unsigned long long targetTick =
                static_cast<unsigned long long>(_adaptiveTimeStepTick + slice + 1u);
            adaptiveDriftKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, nextView, _device->d_adaptiveAcceleration, numParticles, quantum,
                _sphMaxSpeed);
            if (!checkCudaStatus(cudaGetLastError(), "adaptive octree drift launch")) {
                return false;
            }
            adaptiveOctreeCorrectKernel<<<adaptiveNumBlocks, Particle::kDefaultCudaBlockSize>>>(
                nextView, _device->d_adaptiveAcceleration, _device->d_adaptiveLevels,
                _device->d_adaptiveLastForceTicks, numParticles, forceContext, quantum,
                static_cast<int>(levelCount), _adaptiveTimeStepEta,
                std::max(_octreeSoftening, _physicsMinSoftening), deltaTime, targetTick,
                _sphMaxSpeed);
            if (!checkCudaStatus(cudaGetLastError(), "adaptive octree correction launch")) {
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
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "adaptive octree sync")) {
            return false;
        }
        _adaptiveTimeStepTick += sliceCount;
        _device->_leapfrogPrimed = false;
        _device->_hostStateDirty = true;
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
        if (thermalActive) {
            if (!syncHostState()) {
                return false;
            }

            applyThermalModel(deltaTime);
            syncDeviceState();
        }
        if (!_adaptiveTimeStepMarkerPrinted) {
            fprintf(stderr,
                    "[adaptive] backend=cuda_native solver=octree_gpu scheduler=dyadic "
                    "max_level=%u eta=%.4f tree_rebuild=global_step mode=%d\n",
                    levelCount, _adaptiveTimeStepEta, forceContext.mode);
            _adaptiveTimeStepMarkerPrinted = true;
        }
        publishMappedMetrics(deltaTime);
        return true;
    }

    return false;
}
