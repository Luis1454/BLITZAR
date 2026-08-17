/*
 * @file engine/physics/cuda/integration/CudOctreeGpuIntegrators.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Advance regular GPU Octree and TreePM integrators.
 */

/*
 * Module: cuda
 * Responsibility: Execute Euler and Leapfrog Octree force paths.
 */

bool ParticleSystem::updateOctreeGpuIntegrators(float deltaTime, const ForceLawPolicy& forceLaw,
                                                bool thermalActive, OctreeGpuUpdateContext& context)
{
    auto& currentView = context.currentView;
    auto& nextView = context.nextView;
    auto& numParticles = context.numParticles;
    auto& profileFlashMode = context.profileFlashMode;
    auto& treePmEnabled = context.treePmEnabled;
    auto& treePmHybrid = context.treePmHybrid;
    auto& treePmLocalGrid = context.treePmLocalGrid;
    auto& treePmMaxLocalNeighbors = context.treePmMaxLocalNeighbors;
    auto& treePmGather = context.treePmGather;
    auto& treePmGraphRequested = context.treePmGraphRequested;
    auto& rootIndex = context.rootIndex;
    auto& treePmGrid = context.treePmGrid;
    auto& treePmCutoffSquared = context.treePmCutoffSquared;

    if (_integratorMode == IntegratorMode::Leapfrog) {
        if (!_device->d_k1v || !_device->d_k2v) {
            if (!allocateRk4Buffers(static_cast<int>(_particles.size()))) {
                fprintf(stderr, "[integrator] leapfrog buffers missing\n");
                return false;
            }
        }
        if (!_device->d_vHalf) {
            fprintf(stderr, "[integrator] leapfrog v_half buffer missing\n");
            return false;
        }
    }

    if (_integratorMode == IntegratorMode::Leapfrog) {
        const int openingCriterion =
            _octreeOpeningCriterion == OctreeOpeningCriterion::Bounds ? 1 : 0;
        const int numBlocks =
            (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
        auto* halfVelocity = reinterpret_cast<float3*>(_device->d_vHalf.get());

        bool treePmLeapfrogCompleted = false;
        if (!_device->_leapfrogPrimed && treePmEnabled) {
            treepm::computeTreePmAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, _device->d_k1v, numParticles, _device->d_octreeNodeHot,
                _device->d_octreeNodeNav, _device->d_octreeFirstChild, _device->d_octreeLeafStarts,
                _device->d_octreeLeafCounts, rootIndex, _device->g_dOctreeLeafIndices, forceLaw,
                _physicsMaxAcceleration, openingCriterion, treePmGrid, _device->d_treePmAccelX,
                _device->d_treePmAccelY, _device->d_treePmAccelZ, treePmCutoffSquared);
            if (!checkCudaStatus(cudaGetLastError(), "computeTreePmAcceleration kick1 launch")) {
                return false;
            }
            applyKickHalfStepKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, _device->d_k1v, deltaTime, halfVelocity, numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "applyKickHalfStepKernel launch")) {
                return false;
            }
            driftWithHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, halfVelocity, deltaTime, nextView, numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "driftWithHalfVelocityKernel launch")) {
                return false;
            }
            if (!checkCudaStatus(cudaDeviceSynchronize(), "treepm leapfrog drift sync")) {
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

            if (!buildLinearOctreeGpu(currentView, numParticles)) {
                return false;
            }
            if (treePmEnabled &&
                !buildTreePmGrid(currentView, numParticles, &treePmGrid, &treePmCutoffSquared)) {
                return false;
            }

            treepm::computeTreePmAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, _device->d_k2v, numParticles, _device->d_octreeNodeHot,
                _device->d_octreeNodeNav, _device->d_octreeFirstChild, _device->d_octreeLeafStarts,
                _device->d_octreeLeafCounts, _device->_gpuOctreeRootIndex,
                _device->g_dOctreeLeafIndices, forceLaw, _physicsMaxAcceleration, openingCriterion,
                treePmGrid, _device->d_treePmAccelX, _device->d_treePmAccelY, _device->d_treePmAccelZ,
                treePmCutoffSquared);
            if (!checkCudaStatus(cudaGetLastError(), "computeTreePmAcceleration kick2 launch")) {
                return false;
            }

            finalizeLeapfrogKickKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, halfVelocity, _device->d_k2v, deltaTime, nextView, halfVelocity,
                numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "finalizeLeapfrogKickKernel launch")) {
                return false;
            }
            if (!checkCudaStatus(cudaDeviceSynchronize(), "treepm leapfrog finalize sync")) {
                return false;
            }

            std::swap(_device->d_soaPosX, _device->d_soaNextPosX);
            std::swap(_device->d_soaPosY, _device->d_soaNextPosY);
            std::swap(_device->d_soaPosZ, _device->d_soaNextPosZ);
            std::swap(_device->d_soaVelX, _device->d_soaNextVelX);
            std::swap(_device->d_soaVelY, _device->d_soaNextVelY);
            std::swap(_device->d_soaVelZ, _device->d_soaNextVelZ);
            treePmLeapfrogCompleted = true;
        }
        if (!treePmLeapfrogCompleted) {
            if (!_device->_leapfrogPrimed) {
                primeHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                    currentView, halfVelocity, numParticles);
                if (!checkCudaStatus(cudaGetLastError(), "primeHalfVelocityKernel launch")) {
                    return false;
                }
                _device->_leapfrogPrimed = true;
            }

            computeOctreeAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, _device->d_k1v, numParticles, _device->d_octreeNodeHot,
                _device->d_octreeNodeNav, _device->d_octreeFirstChild, _device->d_octreeLeafStarts,
                _device->d_octreeLeafCounts, rootIndex, _device->g_dOctreeLeafIndices, forceLaw,
                _physicsMaxAcceleration, openingCriterion, 0.0f);
            if (!checkCudaStatus(cudaGetLastError(), "computeOctreeAcceleration kick1 launch")) {
                return false;
            }

            applyKickHalfStepKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, _device->d_k1v, deltaTime, halfVelocity, numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "applyKickHalfStepKernel launch")) {
                return false;
            }

            driftWithHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, halfVelocity, deltaTime, nextView, numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "driftWithHalfVelocityKernel launch")) {
                return false;
            }
            if (!checkCudaStatus(cudaDeviceSynchronize(), "leapfrog drift sync")) {
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

            if (!buildLinearOctreeGpu(currentView, numParticles)) {
                return false;
            }
            const int nextRootIndex = _device->_gpuOctreeRootIndex;

            computeOctreeAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, _device->d_k2v, numParticles, _device->d_octreeNodeHot,
                _device->d_octreeNodeNav, _device->d_octreeFirstChild, _device->d_octreeLeafStarts,
                _device->d_octreeLeafCounts, nextRootIndex, _device->g_dOctreeLeafIndices, forceLaw,
                _physicsMaxAcceleration, openingCriterion, 0.0f);
            if (!checkCudaStatus(cudaGetLastError(), "computeOctreeAcceleration kick2 launch")) {
                return false;
            }

            finalizeLeapfrogKickKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, halfVelocity, _device->d_k2v, deltaTime, nextView, halfVelocity,
                numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "finalizeLeapfrogKickKernel launch")) {
                return false;
            }
            if (!checkCudaStatus(cudaDeviceSynchronize(), "leapfrog finalize sync")) {
                return false;
            }

            std::swap(_device->d_soaPosX, _device->d_soaNextPosX);
            std::swap(_device->d_soaPosY, _device->d_soaNextPosY);
            std::swap(_device->d_soaPosZ, _device->d_soaNextPosZ);
            std::swap(_device->d_soaVelX, _device->d_soaNextVelX);
            std::swap(_device->d_soaVelY, _device->d_soaNextVelY);
            std::swap(_device->d_soaVelZ, _device->d_soaNextVelZ);
        }
    }
    else if (_integratorMode == IntegratorMode::Euler) {
        const auto forceStartTime = std::chrono::high_resolution_clock::now();
        const int numBlocks =
            (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;
        if (treePmHybrid) {
            const int treePmCellRadius =
                std::clamp(static_cast<int>(
                               std::ceil(std::sqrt(treePmCutoffSquared) * treePmGrid.invCellSize)),
                           1, 2);
            treepm::
                updateParticlesTreePmHybridKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                    currentView, nextView, numParticles, _device->d_octreeNodeHot,
                    _device->d_octreeNodeNav, _device->d_octreeFirstChild, _device->d_octreeLeafStarts,
                    _device->d_octreeLeafCounts, rootIndex, _device->g_dOctreeLeafIndices, forceLaw,

                    deltaTime, _physicsMaxAcceleration,
                    _octreeOpeningCriterion == OctreeOpeningCriterion::Bounds ? 1 : 0, treePmGrid,
                    _device->d_sphSortedIndex, _device->d_sphCellStart, _device->d_sphCellEnd,
                    _device->d_treePmAccelX, _device->d_treePmAccelY, _device->d_treePmAccelZ,
                    _device->d_treePmCellMask, treePmCutoffSquared, treePmCellRadius,
                    treePmMaxLocalNeighbors, std::max(_treePmDenseCellThreshold, 1),
                    treePmGather ? _device->d_treePmSortedPosX.get() : nullptr,
                    treePmGather ? _device->d_treePmSortedPosY.get() : nullptr,
                    treePmGather ? _device->d_treePmSortedPosZ.get() : nullptr,
                    treePmGather ? _device->d_treePmSortedMass.get() : nullptr);
            if (!checkCudaStatus(cudaGetLastError(), "updateParticlesTreePmHybrid kernel launch")) {
                return false;
            }
        }
        else if (treePmLocalGrid) {
            const int treePmCellRadius =
                std::clamp(static_cast<int>(
                               std::ceil(std::sqrt(treePmCutoffSquared) * treePmGrid.invCellSize)),
                           1, 2);
            treepm::updateParticlesTreePmLocalGridKernel<<<numBlocks,
                                                           Particle::kDefaultCudaBlockSize>>>(
                currentView, nextView, numParticles, treePmGrid, _device->d_sphSortedIndex,
                _device->d_sphCellStart, _device->d_sphCellEnd, forceLaw, deltaTime,
                _physicsMaxAcceleration, _device->d_treePmAccelX, _device->d_treePmAccelY,
                _device->d_treePmAccelZ, _device->d_treePmCellMask, treePmCutoffSquared,
                treePmCellRadius, treePmMaxLocalNeighbors,
                treePmGather ? _device->d_treePmSortedPosX.get() : nullptr,
                treePmGather ? _device->d_treePmSortedPosY.get() : nullptr,
                treePmGather ? _device->d_treePmSortedPosZ.get() : nullptr,
                treePmGather ? _device->d_treePmSortedMass.get() : nullptr);
            if (!checkCudaStatus(cudaGetLastError(),
                                 "updateParticlesTreePmLocalGrid kernel launch")) {
                return false;
            }
        }
        else if (treePmEnabled) {
            treepm::updateParticlesTreePmKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, nextView, numParticles, _device->d_octreeNodeHot,
                _device->d_octreeNodeNav, _device->d_octreeFirstChild, _device->d_octreeLeafStarts,
                _device->d_octreeLeafCounts, rootIndex, _device->g_dOctreeLeafIndices, forceLaw,
                deltaTime, _physicsMaxAcceleration,
                _octreeOpeningCriterion == OctreeOpeningCriterion::Bounds ? 1 : 0, treePmGrid,
                _device->d_treePmAccelX, _device->d_treePmAccelY, _device->d_treePmAccelZ,
                treePmCutoffSquared);
            if (!checkCudaStatus(cudaGetLastError(), "updateParticlesTreePm kernel launch")) {
                return false;
            }
        }
        else {
            updateParticlesOctree<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, nextView, numParticles, _device->d_octreeNodeHot,
                _device->d_octreeNodeNav, _device->d_octreeFirstChild, _device->d_octreeLeafStarts,
                _device->d_octreeLeafCounts, rootIndex, _device->g_dOctreeLeafIndices, forceLaw,
                deltaTime, _physicsMaxAcceleration,
                _octreeOpeningCriterion == OctreeOpeningCriterion::Bounds ? 1 : 0, 0.0f);
            if (!checkCudaStatus(cudaGetLastError(), "updateParticlesOctree kernel launch")) {
                return false;
            }
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "updateParticlesOctree kernel sync")) {
            return false;
        }
        if (profileFlashMode) {
            const auto forceStopTime = std::chrono::high_resolution_clock::now();
            const double forceMs =
                std::chrono::duration<double, std::milli>(forceStopTime - forceStartTime).count();
            fprintf(stderr, "[octree-profile] computeBarnesHutForce_ms=%.3f\n", forceMs);
        }

        // Swap buffers
        std::swap(_device->d_soaPosX, _device->d_soaNextPosX);

        std::swap(_device->d_soaPosY, _device->d_soaNextPosY);
        std::swap(_device->d_soaPosZ, _device->d_soaNextPosZ);
        std::swap(_device->d_soaVelX, _device->d_soaNextVelX);
        std::swap(_device->d_soaVelY, _device->d_soaNextVelY);
        std::swap(_device->d_soaVelZ, _device->d_soaNextVelZ);
        _device->_leapfrogPrimed = false;
        if (treePmGraphRequested && _device->_treePmGraphCaptured[_device->_treePmGraphSlot]) {
            _device->_treePmGraphSlot ^= 1;
        }
    }

    return finalizeOctreeGpuUpdate(deltaTime, thermalActive, context);
}
