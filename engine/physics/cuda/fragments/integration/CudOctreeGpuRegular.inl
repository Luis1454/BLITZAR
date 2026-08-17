/*
 * @file engine/physics/cuda/fragments/integration/CudOctreeGpuRegular.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Prepare regular GPU Octree and TreePM force evaluation.
 */

/*
 * Module: cuda
 * Responsibility: Build the spatial structures and select the integration phase.
 */

bool ParticleSystem::updateOctreeGpuRegular(float deltaTime, const ForceLawPolicy& forceLaw,
                                            bool thermalActive, OctreeGpuUpdateContext& context)
{
    auto& currentView = context.currentView;
    auto& nextView = context.nextView;
    auto& numParticles = context.numParticles;
    auto& treePmEnabled = context.treePmEnabled;
    auto& treePmHybrid = context.treePmHybrid;
    auto& treePmLocalGrid = context.treePmLocalGrid;
    auto& treePmMaxLocalNeighbors = context.treePmMaxLocalNeighbors;
    auto& treePmNeighborGrid = context.treePmNeighborGrid;
    auto& treePmGather = context.treePmGather;
    auto& treePmMorton = context.treePmMorton;
    auto& treePmGraphRequested = context.treePmGraphRequested;
    auto& rootIndex = context.rootIndex;
    auto& treePmGrid = context.treePmGrid;
    auto& treePmCutoffSquared = context.treePmCutoffSquared;

    const cudaFuncCache cachePreference =
        _cudaCachePreference == "shared"    ? cudaFuncCachePreferShared
        : _cudaCachePreference == "default" ? cudaFuncCachePreferNone
                                            : cudaFuncCachePreferL1;
    if (!checkCudaStatus(cudaFuncSetCacheConfig(computeOctreeAccelerationKernel, cachePreference),
                         "computeOctreeAccelerationKernel cache config")) {
        return false;
    }
    if (!checkCudaStatus(cudaFuncSetCacheConfig(updateParticlesOctree, cachePreference),
                         "updateParticlesOctree cache config")) {
        return false;
    }

    if ((!treePmLocalGrid || treePmHybrid) && !buildLinearOctreeGpu(currentView, numParticles)) {
        return false;
    }
    rootIndex = _device->_gpuOctreeRootIndex;
    if (treePmEnabled) {
        if (!buildTreePmGrid(currentView, numParticles, &treePmGrid, &treePmCutoffSquared)) {
            return false;
        }
        treePmGather = treePmNeighborGrid && treePmGatherEnabled();
        treePmMorton = treePmNeighborGrid && treePmMortonEnabled();
        if (!_device->_treePmMarkerPrinted) {
            fprintf(stderr,
                    "[treepm] enabled solver=%s precision=fp32 requested_precision=%s model=%s "
                    "assignment=%s grid=%d jacobi=%d local_grid=%d neighbors=%d "
                    "pm_particles=%d dense_threshold=%d cutoff2=%.6f cache=%s gather=%d "
                    "morton=%d\n",
                    _device->_treePmFftActive ? "fft" : "red_black", _treePmPrecision.c_str(),

                    _treePmModel.c_str(), _treePmAssignment.c_str(), treePmGrid.gridSize,
                    _treePmJacobiIterations, treePmLocalGrid ? 1 : 0, treePmMaxLocalNeighbors,
                    numParticles, _treePmDenseCellThreshold, treePmCutoffSquared,
                    _cudaCachePreference.c_str(), treePmGather ? 1 : 0, treePmMorton ? 1 : 0);
            _device->_treePmMarkerPrinted = true;
        }
        if (treePmGraphRequested && !_device->_treePmGraphCaptured[_device->_treePmGraphSlot]) {
            if (!captureTreePmGraph(_device->_treePmGraphSlot, currentView, nextView, numParticles,
                                    _treePmParticleLimit <= 0
                                        ? numParticles
                                        : std::min(_treePmParticleLimit, numParticles),
                                    treePmGrid, treePmCutoffSquared, forceLaw, deltaTime,
                                    _physicsMaxAcceleration)) {
                fprintf(stderr, "[treepm] cuda_graph=capture_failed fallback=regular\n");
            }
        }
        if (treePmNeighborGrid && !buildTreePmNeighborGrid(currentView, numParticles, treePmGrid)) {
            return false;
        }
    }

    if (_adaptiveTimeStepsEnabled && !_adaptiveTimeStepCostGuard) {
        return updateOctreeGpuAdaptive(deltaTime, forceLaw, thermalActive, context);
    }

    return updateOctreeGpuIntegrators(deltaTime, forceLaw, thermalActive, context);
}
