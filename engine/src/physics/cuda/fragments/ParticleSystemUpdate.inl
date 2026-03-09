bool ParticleSystem::update(float deltaTime) {
    const float softening = std::max(1e-4f, _octreeSoftening);
    const bool thermalActive = (_thermalHeatingCoeff > 0.0f || _thermalRadiationCoeff > 0.0f);
    auto syncParticlesFromDevice = [&]() -> bool {
        return syncHostState();
    };
    auto applySphCorrection = [&](bool uploadHostState) -> bool {
        if (!_sphEnabled) {
            return true;
        }
        if (!d_particles || !d_sphDensity || !d_sphPressure) {
            return false;
        }
        const int numParticles = static_cast<int>(_particles.size());
        if (numParticles < 2) {
            return true;
        }
        const int numBlocks = (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;

        if (uploadHostState) {
            if (!checkCudaStatus(
                    cudaMemcpy(d_particles, _particles.data(), _particles.size() * sizeof(Particle), cudaMemcpyHostToDevice),
                    "cudaMemcpy(HtoD particles sph-corr)")) {
                return false;
            }
        }

        computeSphDensityPressureKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            d_particles,
            d_sphDensity,
            d_sphPressure,
            numParticles,
            _sphSmoothingLength,
            _sphRestDensity,
            _sphGasConstant
        );
        if (!checkCudaStatus(cudaGetLastError(), "computeSphDensityPressure kernel launch")) {
            return false;
        }

        constexpr float kSphCorrectionScale = 0.22f;
        integrateSphKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            d_particles,
            d_sphDensity,
            d_sphPressure,
            numParticles,
            _sphSmoothingLength,
            _sphViscosity,
            deltaTime,
            kSphCorrectionScale
        );
        if (!checkCudaStatus(cudaGetLastError(), "integrateSph kernel launch")) {
            return false;
        } 
        if (!checkCudaStatus(cudaDeviceSynchronize(), "sph kernels sync")) {
            return false;
        }
        _hostStateDirty = true;
        return true;
    };

    if (_solverMode == SolverMode::OctreeCpu) {
        if (!syncParticlesFromDevice()) {
            return false;
        }
        if (_integratorMode != IntegratorMode::Rk4) {
            _octree.build(_particles);
            if (_octreeForces.size() != _particles.size()) {
                _octreeForces.resize(_particles.size(), Vector3(0.0f, 0.0f, 0.0f));
            }

            for (size_t i = 0; i < _particles.size(); ++i) {
                _octreeForces[i] = _octree.computeForceOn(_particles[i], i, _octreeTheta, _octreeSoftening);
                _octreeForces[i] = clampAcceleration(_octreeForces[i], kGravityMaxAcceleration);
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
                outAcc[i] = _octree.computeForceOn(state[i], i, _octreeTheta, _octreeSoftening);
                outAcc[i] = clampAcceleration(outAcc[i], kGravityMaxAcceleration);
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
        return true;
    }

    if (_solverMode == SolverMode::OctreeGpu) {
        if (_integratorMode == IntegratorMode::Rk4) {
            static bool warnedRk4OctreeGpu = false;
            if (!warnedRk4OctreeGpu) {
                fprintf(stderr, "[integrator] rk4 is not implemented for octree_gpu, using euler\n");
                warnedRk4OctreeGpu = true;
            }
        }
        if (!d_particles) {
            return false;
        }
        if (!syncParticlesFromDevice()) {
            return false;
        }
        _octree.build(_particles);
        _octree.exportGpu(_octreeGpuNodes, _octreeGpuLeafIndices);
        const int rootIndex = _octree.getRootIndex();
        if (rootIndex < 0 || _octreeGpuNodes.empty()) {
            return false;
        }

        if (!checkCudaStatus(
                cudaMemcpy(last, d_particles, _particles.size() * sizeof(Particle), cudaMemcpyDeviceToDevice),
                "cudaMemcpy(DtoD base octree_gpu)")) {
            return false;
        }

        if (g_dOctreeNodeCapacity < _octreeGpuNodes.size()) {
            if (g_dOctreeNodes) {
                cudaFree(g_dOctreeNodes);
                g_dOctreeNodes = nullptr;
            }
            if (!checkCudaStatus(cudaMalloc(&g_dOctreeNodes, _octreeGpuNodes.size() * sizeof(GpuOctreeNode)), "cudaMalloc(d_octree_nodes)")) {
                g_dOctreeNodeCapacity = 0;
                return false;
            }
            g_dOctreeNodeCapacity = _octreeGpuNodes.size();
        }
        if (g_dOctreeLeafCapacity < _octreeGpuLeafIndices.size()) {
            if (g_dOctreeLeafIndices) {
                cudaFree(g_dOctreeLeafIndices);
                g_dOctreeLeafIndices = nullptr;
            }
            if (!checkCudaStatus(cudaMalloc(&g_dOctreeLeafIndices, _octreeGpuLeafIndices.size() * sizeof(int)), "cudaMalloc(d_octree_leaf_indices)")) {
                g_dOctreeLeafCapacity = 0;
                return false;
            }
            g_dOctreeLeafCapacity = _octreeGpuLeafIndices.size();
        }

        if (!checkCudaStatus(
                cudaMemcpy(g_dOctreeNodes, _octreeGpuNodes.data(), _octreeGpuNodes.size() * sizeof(GpuOctreeNode), cudaMemcpyHostToDevice),
                "cudaMemcpy(HtoD octree nodes)")) {
            return false;
        }
        if (!_octreeGpuLeafIndices.empty()) {
            if (!checkCudaStatus(
                    cudaMemcpy(g_dOctreeLeafIndices, _octreeGpuLeafIndices.data(), _octreeGpuLeafIndices.size() * sizeof(int), cudaMemcpyHostToDevice),
                    "cudaMemcpy(HtoD octree leaf indices)")) {
                return false;
            }
        }

        updateParticlesOctree<<<(_particles.size() + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize, Particle::kDefaultCudaBlockSize>>>(
            last,
            d_particles,
            static_cast<int>(_particles.size()),
            g_dOctreeNodes,
            rootIndex,
            g_dOctreeLeafIndices,
            _octreeTheta,
            softening,
            deltaTime
        );
        if (!checkCudaStatus(cudaGetLastError(), "updateParticlesOctree kernel launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "updateParticlesOctree kernel sync")) {
            return false;
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

    if (!d_particles || !last) {
        return false;
    }

    const int numParticles = static_cast<int>(_particles.size());
    const int numBlocks = (numParticles + Particle::kDefaultCudaBlockSize - 1) / Particle::kDefaultCudaBlockSize;

    if (!checkCudaStatus(
            cudaMemcpy(last, d_particles, _particles.size() * sizeof(Particle), cudaMemcpyDeviceToDevice),
            "cudaMemcpy(DtoD base)")) {
        return false;
    }

    if (_integratorMode == IntegratorMode::Rk4) {
        if (!d_stage || !d_k1x || !d_k1v) {
            if (!allocateRk4Buffers(numParticles)) {
                fprintf(stderr, "[integrator] rk4 buffers missing, falling back to euler\n");
                _integratorMode = IntegratorMode::Euler;
            }
        }
    }

    if (_integratorMode == IntegratorMode::Rk4) {
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(last, d_k1x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k1 launch")) {
            return false;
        }
        computePairwiseAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(last, d_k1v, numParticles, softening, kGravityMaxAcceleration);
        if (!checkCudaStatus(cudaGetLastError(), "computeAcceleration k1 launch")) {
            return false;
        }

        buildRk4StageKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(last, d_k1x, d_k1v, 0.5f * deltaTime, d_stage, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k2 launch")) {
            return false;
        }
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(d_stage, d_k2x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k2 launch")) {
            return false;
        }
        computePairwiseAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(d_stage, d_k2v, numParticles, softening, kGravityMaxAcceleration);
        if (!checkCudaStatus(cudaGetLastError(), "computeAcceleration k2 launch")) {
            return false;
        }

        buildRk4StageKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(last, d_k2x, d_k2v, 0.5f * deltaTime, d_stage, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k3 launch")) {
            return false;
        }
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(d_stage, d_k3x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k3 launch")) {
            return false;
        }
        computePairwiseAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(d_stage, d_k3v, numParticles, softening, kGravityMaxAcceleration);
        if (!checkCudaStatus(cudaGetLastError(), "computeAcceleration k3 launch")) {
            return false;
        }

        buildRk4StageKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(last, d_k3x, d_k3v, deltaTime, d_stage, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k4 launch")) {
            return false;
        }
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(d_stage, d_k4x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k4 launch")) {
            return false;
        }
        computePairwiseAccelerationKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(d_stage, d_k4v, numParticles, softening, kGravityMaxAcceleration);
        if (!checkCudaStatus(cudaGetLastError(), "computeAcceleration k4 launch")) {
            return false;
        }

        finalizeRk4Kernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            last,
            d_k1x, d_k2x, d_k3x, d_k4x,
            d_k1v, d_k2v, d_k3v, d_k4v,
            deltaTime,
            d_particles,
            numParticles
        );
        if (!checkCudaStatus(cudaGetLastError(), "finalizeRk4 launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "rk4 kernel sync")) {
            return false;
        }
    } else {
        updateParticles<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(last, d_particles, numParticles, deltaTime, softening, kGravityMaxAcceleration);
        if (!checkCudaStatus(cudaGetLastError(), "updateParticles kernel launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "updateParticles kernel sync")) {
            return false;
        }
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

    if constexpr (kProfileLogsEnabled) {
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
        float milliseconds = 0;
        cudaEventElapsedTime(&milliseconds, start, stop);
        printf("Time elapsed: %f ms (%f fps) for computing %zu particles\n", milliseconds, 1000.0f / milliseconds, _particles.size());
    }
    return true;
}

void destroyParticles(ParticleHandle particles) {
    if (d_particles) {
        cudaFree(d_particles);
        d_particles = nullptr;
    }
    if (last) {
        cudaFree(last);
        last = nullptr;
    }
    releaseRk4Buffers();
    releaseSphBuffers();

    (void)particles;
}
