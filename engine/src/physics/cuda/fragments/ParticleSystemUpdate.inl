/*
 * Module: physics/cuda
 * Responsibility: Advance the particle system for one deterministic update step.
 */

#include <chrono>

bool ParticleSystem::update(float deltaTime) {
    const ForceLawPolicy forceLaw = resolveForceLawPolicy(
        _octreeTheta,
        _octreeSoftening,
        _physicsMinSoftening,
        _physicsMinDistance2,
        _physicsMinTheta);
    const bool thermalActive = (_thermalHeatingCoeff > 0.0f || _thermalRadiationCoeff > 0.0f);
    auto syncParticlesFromDevice = [&]() -> bool {
        return syncHostState();
    };
    auto applySphCorrection = [&](bool uploadHostState) -> bool {
        if (!_sphEnabled) {
            return true;
        }
        if (!d_soaPosX || !d_sphDensity || !d_sphPressure) {
            return false;
        }
        const int numParticles = static_cast<int>(_particles.size());
        if (numParticles < 2) {
            return true;
        }
        const int numBlocks = (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;

        if (uploadHostState) {
            syncDeviceState();
        }

        // Build spatial hash grid for neighbor lookup.
        if (!buildSphGrid(numParticles)) {
            return false;
        }

        SphGridParams grid;
        grid.gridSize = _sphGridSize;
        grid.totalCells = _sphGridTotalCells;
        grid.cellSize = std::max(0.01f, _sphSmoothingLength);
        
        ParticleSoAView currentView = getSoAView(false);
        ParticleSoAView nextView = getSoAView(true);

        computeSphDensityPressureGridKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView,
            d_sphDensity,
            d_sphPressure,
            numParticles,
            _sphSmoothingLength,
            _sphRestDensity,
            _sphGasConstant,
            d_sphCellHash,
            d_sphSortedIndex,
            d_sphCellStart,
            d_sphCellEnd,
            grid
        );
        if (!checkCudaStatus(cudaGetLastError(), "computeSphDensityPressureGrid kernel launch")) {
            return false;
        }

        constexpr float kSphCorrectionScale = 0.22f;
        integrateSphGridKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView,
            nextView,
            d_sphDensity,
            d_sphPressure,
            numParticles,
            _sphSmoothingLength,
            _sphViscosity,
            deltaTime,
            kSphCorrectionScale,
            d_sphCellHash,
            d_sphSortedIndex,
            d_sphCellStart,
            d_sphCellEnd,
            grid,
            _sphMaxAcceleration,
            _sphMaxSpeed
        );
        if (!checkCudaStatus(cudaGetLastError(), "integrateSphGrid kernel launch")) {
            return false;
        } 
        if (!checkCudaStatus(cudaDeviceSynchronize(), "sph grid kernels sync")) {
            return false;
        }

        std::swap(d_soaPosX, d_soaNextPosX);
        std::swap(d_soaPosY, d_soaNextPosY);
        std::swap(d_soaPosZ, d_soaNextPosZ);
        std::swap(d_soaVelX, d_soaNextVelX);
        std::swap(d_soaVelY, d_soaNextVelY);
        std::swap(d_soaVelZ, d_soaNextVelZ);

        _hostStateDirty = true;
        return true;
    };

    if (_solverMode == SolverMode::OctreeCpu) {
        if (!syncParticlesFromDevice()) {
            return false;
        }
        if (_integratorMode == IntegratorMode::Euler) {
            _octree.build(_particles);
            if (_octreeForces.size() != _particles.size()) {
                _octreeForces.resize(_particles.size(), Vector3(0.0f, 0.0f, 0.0f));
            }

            for (size_t i = 0; i < _particles.size(); ++i) {
                _octreeForces[i] = _octree.computeForceOn(_particles[i], i, forceLaw, _octreeOpeningCriterion);
                _octreeForces[i] = clampAcceleration(_octreeForces[i], _physicsMaxAcceleration);
                _particles[i].setPressure(_octreeForces[i] * 100.0f);
            }
            for (size_t i = 0; i < _particles.size(); ++i) {
                _particles[i].setVelocity(_particles[i].getVelocity() + _octreeForces[i] * deltaTime);
                _particles[i].setPosition(_particles[i].getPosition() + _particles[i].getVelocity() * deltaTime);
            }
            if (!applySphCorrection(true)) {
                return false;
            }
            if (_sphEnabled && !syncParticlesFromDevice()) {
                return false;
            }
            if (thermalActive) {
                applyThermalModel(deltaTime);
            }
            syncDeviceState();
            publishMappedMetrics(deltaTime);
            return true;
        }

        if (_integratorMode == IntegratorMode::Leapfrog) {
            const size_t n = _particles.size();
            std::vector<Vector3> accStart(n);
            std::vector<Vector3> accEnd(n);
            std::vector<Particle> stage = _particles;

            _octree.build(_particles);
            for (size_t i = 0; i < n; ++i) {
                accStart[i] = _octree.computeForceOn(_particles[i], i, forceLaw, _octreeOpeningCriterion);
                accStart[i] = clampAcceleration(accStart[i], _physicsMaxAcceleration);
            }

            for (size_t i = 0; i < n; ++i) {
                const Vector3 velHalf = _particles[i].getVelocity() + accStart[i] * (0.5f * deltaTime);
                stage[i].setVelocity(velHalf);
                stage[i].setPosition(_particles[i].getPosition() + velHalf * deltaTime);
            }

            _octree.build(stage);
            for (size_t i = 0; i < n; ++i) {
                accEnd[i] = _octree.computeForceOn(stage[i], i, forceLaw, _octreeOpeningCriterion);
                accEnd[i] = clampAcceleration(accEnd[i], _physicsMaxAcceleration);
            }

            for (size_t i = 0; i < n; ++i) {
                const Vector3 velHalf = _particles[i].getVelocity() + accStart[i] * (0.5f * deltaTime);
                const Vector3 nextVel = velHalf + accEnd[i] * (0.5f * deltaTime);
                _particles[i].setPosition(stage[i].getPosition());
                _particles[i].setVelocity(nextVel);
                _particles[i].setPressure(accEnd[i] * 100.0f);
            }
            if (!applySphCorrection(true)) {
                return false;
            }
            if (_sphEnabled && !syncParticlesFromDevice()) {
                return false;
            }
            if (thermalActive) {
                applyThermalModel(deltaTime);
            }
            syncDeviceState();
            publishMappedMetrics(deltaTime);
            return true;
        }

        const size_t n = _particles.size();
        std::vector<Vector3> k1x(n), k2x(n), k3x(n), k4x(n);
        std::vector<Vector3> k1v(n), k2v(n), k3v(n), k4v(n);
        std::vector<Particle> stage(n);
        auto resetStage = [&]() {
            for (size_t i = 0; i < n; ++i) {
                stage[i] = _particles[i];
            }
        };

        auto computeOctreeAcceleration = [&](const std::vector<Particle> &state, std::vector<Vector3> &outAcc) {
            _octree.build(state);
            for (size_t i = 0; i < state.size(); ++i) {
                outAcc[i] = _octree.computeForceOn(state[i], i, forceLaw, _octreeOpeningCriterion);
                outAcc[i] = clampAcceleration(outAcc[i], _physicsMaxAcceleration);
            }
        };

        for (size_t i = 0; i < n; ++i) {
            k1x[i] = _particles[i].getVelocity();
        }
        computeOctreeAcceleration(_particles, k1v);

        resetStage();
        for (size_t i = 0; i < n; ++i) {
            stage[i].setPosition(_particles[i].getPosition() + k1x[i] * (0.5f * deltaTime));
            stage[i].setVelocity(_particles[i].getVelocity() + k1v[i] * (0.5f * deltaTime));
            k2x[i] = stage[i].getVelocity();
        }
        computeOctreeAcceleration(stage, k2v);

        resetStage();
        for (size_t i = 0; i < n; ++i) {
            stage[i].setPosition(_particles[i].getPosition() + k2x[i] * (0.5f * deltaTime));
            stage[i].setVelocity(_particles[i].getVelocity() + k2v[i] * (0.5f * deltaTime));
            k3x[i] = stage[i].getVelocity();
        }
        computeOctreeAcceleration(stage, k3v);

        resetStage();
        for (size_t i = 0; i < n; ++i) {
            stage[i].setPosition(_particles[i].getPosition() + k3x[i] * deltaTime);
            stage[i].setVelocity(_particles[i].getVelocity() + k3v[i] * deltaTime);
            k4x[i] = stage[i].getVelocity();
        }
        computeOctreeAcceleration(stage, k4v);

        for (size_t i = 0; i < n; ++i) {
            const Vector3 weightedVel = (k1v[i] + k2v[i] * 2.0f + k3v[i] * 2.0f + k4v[i]) / 6.0f;
            const Vector3 weightedPos = (k1x[i] + k2x[i] * 2.0f + k3x[i] * 2.0f + k4x[i]) / 6.0f;
            _particles[i].setVelocity(_particles[i].getVelocity() + weightedVel * deltaTime);
            _particles[i].setPosition(_particles[i].getPosition() + weightedPos * deltaTime);
            _particles[i].setPressure(weightedVel * 100.0f);
        }
        if (!applySphCorrection(true)) {
            return false;
        }
        if (_sphEnabled && !syncParticlesFromDevice()) {
            return false;
        }
        if (thermalActive) {
            applyThermalModel(deltaTime);
        }
        syncDeviceState();
        publishMappedMetrics(deltaTime);
        return true;
    }

    if (_solverMode == SolverMode::OctreeGpu) {
        const bool profileFlashMode = parseBoolEnv("GRAVITY_OCTREE_PROFILE_FLASH", false);
        if (_integratorMode == IntegratorMode::Rk4) {
            fprintf(stderr, "[integrator] rk4 is not supported with octree_gpu\n");
            return false;
        }
        if (!d_soaPosX) {
            return false;
        }
        ParticleSoAView currentView = getSoAView(false);
        ParticleSoAView nextView = getSoAView(true);

        const int numParticles = static_cast<int>(_particles.size());

        // Prefer L1 for octree node reads in BH traversal kernels.
        if (!checkCudaStatus(
                cudaFuncSetCacheConfig(computeOctreeAccelerationKernel, cudaFuncCachePreferL1),
                "computeOctreeAccelerationKernel cache config")) {
            return false;
        }
        if (!checkCudaStatus(cudaFuncSetCacheConfig(updateParticlesOctree, cudaFuncCachePreferL1),
                             "updateParticlesOctree cache config")) {
            return false;
        }

        if (!buildLinearOctreeGpu(currentView, numParticles)) {
            return false;
        }
        const int rootIndex = _gpuOctreeRootIndex;

        if (_integratorMode == IntegratorMode::Leapfrog) {
            if (!d_k1v || !d_k2v) {
                if (!allocateRk4Buffers(static_cast<int>(_particles.size()))) {
                    fprintf(stderr, "[integrator] leapfrog buffers missing\n");
                    return false;
                }
            }
            if (!d_vHalf) {
                fprintf(stderr, "[integrator] leapfrog v_half buffer missing\n");
                return false;
            }
        }

        if (_integratorMode == IntegratorMode::Leapfrog) {
            const int openingCriterion = _octreeOpeningCriterion == OctreeOpeningCriterion::Bounds ? 1 : 0;
            const int numBlocks = (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;

            if (!_leapfrogPrimed) {
                primeHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(currentView, d_vHalf, numParticles);
                if (!checkCudaStatus(cudaGetLastError(), "primeHalfVelocityKernel launch")) {
                    return false;
                }
                _leapfrogPrimed = true;
            }

            computeOctreeAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView,
                d_k1v,
                numParticles,
                g_dOctreeNodes,
                rootIndex,
                g_dOctreeLeafIndices,
                forceLaw,
                _physicsMaxAcceleration,
                openingCriterion
            );
            if (!checkCudaStatus(cudaGetLastError(), "computeOctreeAcceleration kick1 launch")) {
                return false;
            }

            applyKickHalfStepKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView,
                d_k1v,
                deltaTime,
                d_vHalf,
                numParticles
            );
            if (!checkCudaStatus(cudaGetLastError(), "applyKickHalfStepKernel launch")) {
                return false;
            }

            driftWithHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView,
                d_vHalf,
                deltaTime,
                nextView,
                numParticles
            );
            if (!checkCudaStatus(cudaGetLastError(), "driftWithHalfVelocityKernel launch")) {
                return false;
            }
            if (!checkCudaStatus(cudaDeviceSynchronize(), "leapfrog drift sync")) {
                return false;
            }

            std::swap(d_soaPosX, d_soaNextPosX);
            std::swap(d_soaPosY, d_soaNextPosY);
            std::swap(d_soaPosZ, d_soaNextPosZ);
            std::swap(d_soaVelX, d_soaNextVelX);
            std::swap(d_soaVelY, d_soaNextVelY);
            std::swap(d_soaVelZ, d_soaNextVelZ);

            currentView = getSoAView(false);
            nextView = getSoAView(true);

            if (!buildLinearOctreeGpu(currentView, numParticles)) {
                return false;
            }
            const int nextRootIndex = _gpuOctreeRootIndex;

            computeOctreeAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView,
                d_k2v,
                numParticles,
                g_dOctreeNodes,
                nextRootIndex,
                g_dOctreeLeafIndices,
                forceLaw,
                _physicsMaxAcceleration,
                openingCriterion
            );
            if (!checkCudaStatus(cudaGetLastError(), "computeOctreeAcceleration kick2 launch")) {
                return false;
            }

            finalizeLeapfrogKickKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView,
                d_vHalf,
                d_k2v,
                deltaTime,
                nextView,
                d_vHalf,
                numParticles
            );
            if (!checkCudaStatus(cudaGetLastError(), "finalizeLeapfrogKickKernel launch")) {
                return false;
            }
            if (!checkCudaStatus(cudaDeviceSynchronize(), "leapfrog finalize sync")) {
                return false;
            }

            std::swap(d_soaPosX, d_soaNextPosX);
            std::swap(d_soaPosY, d_soaNextPosY);
            std::swap(d_soaPosZ, d_soaNextPosZ);
            std::swap(d_soaVelX, d_soaNextVelX);
            std::swap(d_soaVelY, d_soaNextVelY);
            std::swap(d_soaVelZ, d_soaNextVelZ);
        } else if (_integratorMode == IntegratorMode::Euler) {
            const auto forceStartTime = std::chrono::high_resolution_clock::now();
            updateParticlesOctree<<<(_particles.size() + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize, Particle::kDefaultCudaBlockSize>>>(
                currentView,
                nextView,
                static_cast<int>(_particles.size()),
                g_dOctreeNodes,
                rootIndex,
                g_dOctreeLeafIndices,
                forceLaw,
                deltaTime,
                _physicsMaxAcceleration,
                _octreeOpeningCriterion == OctreeOpeningCriterion::Bounds ? 1 : 0
            );
            if (!checkCudaStatus(cudaGetLastError(), "updateParticlesOctree kernel launch")) {
                return false;
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
            std::swap(d_soaPosX, d_soaNextPosX);
            std::swap(d_soaPosY, d_soaNextPosY);
            std::swap(d_soaPosZ, d_soaNextPosZ);
            std::swap(d_soaVelX, d_soaNextVelX);
            std::swap(d_soaVelY, d_soaNextVelY);
            std::swap(d_soaVelZ, d_soaNextVelZ);
            _leapfrogPrimed = false;
        }

        if (!applySphCorrection(false)) {
            return false;
        }
        _hostStateDirty = true;
        if (thermalActive) {
            if (!syncParticlesFromDevice()) {
                return false;
            }
            applyThermalModel(deltaTime);
            syncDeviceState();
        }
        publishMappedMetrics(deltaTime);
        return true;
    }

    constexpr bool kProfileLogsEnabled = GRAVITY_PROFILE_LOGS != 0;
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    if constexpr (kProfileLogsEnabled) {
        cudaEventCreate(&start);
        cudaEventCreate(&stop);
        cudaEventRecord(start);
    }

    if (!d_soaPosX) {
        return false;
    }

    const int numParticles = static_cast<int>(_particles.size());
    const int numBlocks = (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;

    ParticleSoAView currentView = getSoAView(false);
    ParticleSoAView nextView = getSoAView(true);

    if (_integratorMode == IntegratorMode::Rk4 || _integratorMode == IntegratorMode::Leapfrog) {
        if (!d_stage || !d_k1x || !d_k2x || !d_k3x || !d_k4x || !d_k1v || !d_k2v || !d_k3v || !d_k4v) {
            if (!allocateRk4Buffers(numParticles)) {
                fprintf(stderr, "[integrator] advanced integrator buffers missing\n");
                return false;
            }
        }
        if (_integratorMode == IntegratorMode::Leapfrog && !d_vHalf) {
            if (!allocateRk4Buffers(numParticles)) {
                fprintf(stderr, "[integrator] leapfrog v_half buffer missing\n");
                return false;
            }
        }
    }

    if (_integratorMode == IntegratorMode::Rk4) {
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(currentView, d_k1x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k1 launch")) {
            return false;
        }
        computePairwiseAccelerationKernelTiled<<<numBlocks, Particle::kDefaultCudaBlockSize, 4096>>>(currentView, d_k1v, numParticles, forceLaw, _physicsMaxAcceleration);
        if (!checkCudaStatus(cudaGetLastError(), "computeAcceleration k1 launch (tiled)")) {
            return false;
        }

        buildRk4StageKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(currentView, d_k1x, d_k1v, 0.5f * deltaTime, nextView, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k2 launch")) {
            return false;
        }
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(nextView, d_k2x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k2 launch")) {
            return false;
        }
        computePairwiseAccelerationKernelTiled<<<numBlocks, Particle::kDefaultCudaBlockSize, 4096>>>(nextView, d_k2v, numParticles, forceLaw, _physicsMaxAcceleration);
        if (!checkCudaStatus(cudaGetLastError(), "computeAcceleration k2 launch (tiled)")) {
            return false;
        }

        buildRk4StageKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(currentView, d_k2x, d_k2v, 0.5f * deltaTime, nextView, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k3 launch")) {
            return false;
        }
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(nextView, d_k3x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k3 launch")) {
            return false;
        }
        computePairwiseAccelerationKernelTiled<<<numBlocks, Particle::kDefaultCudaBlockSize, 4096>>>(nextView, d_k3v, numParticles, forceLaw, _physicsMaxAcceleration);
        if (!checkCudaStatus(cudaGetLastError(), "computeAcceleration k3 launch (tiled)")) {
            return false;
        }

        buildRk4StageKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(currentView, d_k3x, d_k3v, deltaTime, nextView, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k4 launch")) {
            return false;
        }
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(nextView, d_k4x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k4 launch")) {
            return false;
        }
        computePairwiseAccelerationKernelTiled<<<numBlocks, Particle::kDefaultCudaBlockSize, 4096>>>(nextView, d_k4v, numParticles, forceLaw, _physicsMaxAcceleration);
        if (!checkCudaStatus(cudaGetLastError(), "computeAcceleration k4 launch (tiled)")) {
            return false;
        }

        finalizeRk4Kernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView,
            d_k1x, d_k2x, d_k3x, d_k4x,
            d_k1v, d_k2v, d_k3v, d_k4v,
            deltaTime,
            nextView,
            numParticles
        );
        if (!checkCudaStatus(cudaGetLastError(), "finalizeRk4 launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "rk4 kernel sync")) {
            return false;
        }
    } else if (_integratorMode == IntegratorMode::Leapfrog) {
        if (!_leapfrogPrimed) {
            primeHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(currentView, d_vHalf, numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "pairwise primeHalfVelocityKernel launch")) {
                return false;
            }
            _leapfrogPrimed = true;
        }

        computePairwiseAccelerationKernelTiled<<<numBlocks, Particle::kDefaultCudaBlockSize, 4096>>>(
            currentView,
            d_k1v,
            numParticles,
            forceLaw,
            _physicsMaxAcceleration);
        if (!checkCudaStatus(cudaGetLastError(), "pairwise leapfrog kick1 acceleration launch (tiled)")) {
            return false;
        }

        applyKickHalfStepKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView,
            d_k1v,
            deltaTime,
            d_vHalf,
            numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "pairwise applyKickHalfStepKernel launch")) {
            return false;
        }

        driftWithHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView,
            d_vHalf,
            deltaTime,
            nextView,
            numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "pairwise driftWithHalfVelocityKernel launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "pairwise leapfrog drift sync")) {
            return false;
        }

        std::swap(d_soaPosX, d_soaNextPosX);
        std::swap(d_soaPosY, d_soaNextPosY);
        std::swap(d_soaPosZ, d_soaNextPosZ);
        std::swap(d_soaVelX, d_soaNextVelX);
        std::swap(d_soaVelY, d_soaNextVelY);
        std::swap(d_soaVelZ, d_soaNextVelZ);

        currentView = getSoAView(false);
        nextView = getSoAView(true);

        computePairwiseAccelerationKernelTiled<<<numBlocks, Particle::kDefaultCudaBlockSize, 4096>>>(
            currentView,
            d_k2v,
            numParticles,
            forceLaw,
            _physicsMaxAcceleration);
        if (!checkCudaStatus(cudaGetLastError(), "pairwise leapfrog kick2 acceleration launch (tiled)")) {
            return false;
        }

        finalizeLeapfrogKickKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView,
            d_vHalf,
            d_k2v,
            deltaTime,
            nextView,
            d_vHalf,
            numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "pairwise finalizeLeapfrogKickKernel launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "pairwise leapfrog finalize sync")) {
            return false;
        }
    } else {
        _leapfrogPrimed = false;
        updateParticles<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(currentView, nextView, numParticles, deltaTime, forceLaw, _physicsMaxAcceleration);
        if (!checkCudaStatus(cudaGetLastError(), "updateParticles kernel launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "updateParticles kernel sync")) {
            return false;
        }
    }

    // Swap buffers
    std::swap(d_soaPosX, d_soaNextPosX);
    std::swap(d_soaPosY, d_soaNextPosY);
    std::swap(d_soaPosZ, d_soaNextPosZ);
    std::swap(d_soaVelX, d_soaNextVelX);
    std::swap(d_soaVelY, d_soaNextVelY);
    std::swap(d_soaVelZ, d_soaNextVelZ);

    if (!applySphCorrection(false)) {
        return false;
    }
    _hostStateDirty = true;
    if (thermalActive) {
        if (!syncParticlesFromDevice()) {
            return false;
        }
        applyThermalModel(deltaTime);
        syncDeviceState();
    }

    if constexpr (kProfileLogsEnabled) {
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        printf("Time elapsed: %f ms (%f fps) for computing %zu particles\n", milliseconds, 1000.0f / milliseconds, _particles.size());
    }
    publishMappedMetrics(deltaTime);
    return true;
}

// Note: destroyParticles logic moved to ParticleSystem destructor and releaseParticleBuffers.
