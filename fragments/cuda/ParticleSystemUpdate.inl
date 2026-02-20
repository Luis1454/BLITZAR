void ParticleSystem::update(float deltaTime) {
    constexpr float kOctreeMaxAcceleration = 64.0f;
    auto applySphCorrection = [&]() {
        if (!_sphEnabled) {
            return;
        }
        if (!d_particles || !d_sphDensity || !d_sphPressure) {
            return;
        }
        const int numParticles = static_cast<int>(_particles.size());
        if (numParticles < 2) {
            return;
        }
        const int numBlocks = (numParticles + BLOCK_SIZE - 1) / BLOCK_SIZE;

        if (!checkCudaStatus(
                cudaMemcpy(d_particles, _particles.data(), _particles.size() * sizeof(Particle), cudaMemcpyHostToDevice),
                "cudaMemcpy(HtoD particles sph-corr)")) {
            return;
        }

        computeSphDensityPressureKernel<<<numBlocks, BLOCK_SIZE>>>(
            d_particles,
            d_sphDensity,
            d_sphPressure,
            numParticles,
            _sphSmoothingLength,
            _sphRestDensity,
            _sphGasConstant
        );
        if (!checkCudaStatus(cudaGetLastError(), "computeSphDensityPressure kernel launch")) {
            return;
        }

        constexpr float kSphCorrectionScale = 0.22f;
        integrateSphKernel<<<numBlocks, BLOCK_SIZE>>>(
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
            return;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "sph kernels sync")) {
            return;
        }

        if (!checkCudaStatus(
                cudaMemcpy(_particles.data(), d_particles, _particles.size() * sizeof(Particle), cudaMemcpyDeviceToHost),
                "cudaMemcpy(DtoH particles sph-corr)")) {
            return;
        }
    };

    if (_solverMode == SolverMode::OctreeCpu) {
        if (_integratorMode != IntegratorMode::Rk4) {
            _octree.build(_particles);
            if (_octreeForces.size() != _particles.size()) {
                _octreeForces.resize(_particles.size(), Vector3(0.0f, 0.0f, 0.0f));
            }

            for (size_t i = 0; i < _particles.size(); ++i) {
                _octreeForces[i] = _octree.computeForceOn(_particles[i], i, _octreeTheta, _octreeSoftening);
                const float accelNorm = _octreeForces[i].norm();
                if (accelNorm > kOctreeMaxAcceleration && accelNorm > 1e-12f) {
                    _octreeForces[i] = _octreeForces[i] * (kOctreeMaxAcceleration / accelNorm);
                }
                _particles[i].setPressure(_octreeForces[i] * 100.0f);
            }
            for (size_t i = 0; i < _particles.size(); ++i) {
                _particles[i].setVelocity(_particles[i].getVelocity() + _octreeForces[i] * deltaTime);
                _particles[i].setPosition(_particles[i].getPosition() + _particles[i].getVelocity() * deltaTime);
            }
            applySphCorrection();
            applyThermalModel(deltaTime);
            return;
        }

        const size_t n = _particles.size();
        std::vector<Vector3> k1x(n), k2x(n), k3x(n), k4x(n);
        std::vector<Vector3> k1v(n), k2v(n), k3v(n), k4v(n);
        std::vector<Particle> stage = _particles;

        auto computeOctreeAcceleration = [&](const std::vector<Particle> &state, std::vector<Vector3> &outAcc) {
            _octree.build(state);
            for (size_t i = 0; i < state.size(); ++i) {
                outAcc[i] = _octree.computeForceOn(state[i], i, _octreeTheta, _octreeSoftening);
                const float accelNorm = outAcc[i].norm();
                if (accelNorm > kOctreeMaxAcceleration && accelNorm > 1e-12f) {
                    outAcc[i] = outAcc[i] * (kOctreeMaxAcceleration / accelNorm);
                }
            }
        };

        for (size_t i = 0; i < n; ++i) {
            k1x[i] = _particles[i].getVelocity();
        }
        computeOctreeAcceleration(_particles, k1v);

        stage = _particles;
        for (size_t i = 0; i < n; ++i) {
            stage[i].setPosition(_particles[i].getPosition() + k1x[i] * (0.5f * deltaTime));
            stage[i].setVelocity(_particles[i].getVelocity() + k1v[i] * (0.5f * deltaTime));
            k2x[i] = stage[i].getVelocity();
        }
        computeOctreeAcceleration(stage, k2v);

        stage = _particles;
        for (size_t i = 0; i < n; ++i) {
            stage[i].setPosition(_particles[i].getPosition() + k2x[i] * (0.5f * deltaTime));
            stage[i].setVelocity(_particles[i].getVelocity() + k2v[i] * (0.5f * deltaTime));
            k3x[i] = stage[i].getVelocity();
        }
        computeOctreeAcceleration(stage, k3v);

        stage = _particles;
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
        applySphCorrection();
        applyThermalModel(deltaTime);
        return;
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
            return;
        }
        _octree.build(_particles);
        _octree.exportGpu(_octreeGpuNodes, _octreeGpuLeafIndices);
        const int rootIndex = _octree.getRootIndex();
        if (rootIndex < 0 || _octreeGpuNodes.empty()) {
            return;
        }

        if (!checkCudaStatus(
                cudaMemcpy(last, _particles.data(), _particles.size() * sizeof(Particle), cudaMemcpyHostToDevice),
                "cudaMemcpy(HtoD base octree_gpu)")) {
            return;
        }

        if (_dOctreeNodeCapacity < _octreeGpuNodes.size()) {
            if (_dOctreeNodes) {
                cudaFree(_dOctreeNodes);
                _dOctreeNodes = nullptr;
            }
            if (!checkCudaStatus(cudaMalloc(&_dOctreeNodes, _octreeGpuNodes.size() * sizeof(GpuOctreeNode)), "cudaMalloc(d_octree_nodes)")) {
                _dOctreeNodeCapacity = 0;
                return;
            }
            _dOctreeNodeCapacity = _octreeGpuNodes.size();
        }
        if (_dOctreeLeafCapacity < _octreeGpuLeafIndices.size()) {
            if (_dOctreeLeafIndices) {
                cudaFree(_dOctreeLeafIndices);
                _dOctreeLeafIndices = nullptr;
            }
            if (!checkCudaStatus(cudaMalloc(&_dOctreeLeafIndices, _octreeGpuLeafIndices.size() * sizeof(int)), "cudaMalloc(d_octree_leaf_indices)")) {
                _dOctreeLeafCapacity = 0;
                return;
            }
            _dOctreeLeafCapacity = _octreeGpuLeafIndices.size();
        }

        if (!checkCudaStatus(
                cudaMemcpy(_dOctreeNodes, _octreeGpuNodes.data(), _octreeGpuNodes.size() * sizeof(GpuOctreeNode), cudaMemcpyHostToDevice),
                "cudaMemcpy(HtoD octree nodes)")) {
            return;
        }
        if (!_octreeGpuLeafIndices.empty()) {
            if (!checkCudaStatus(
                    cudaMemcpy(_dOctreeLeafIndices, _octreeGpuLeafIndices.data(), _octreeGpuLeafIndices.size() * sizeof(int), cudaMemcpyHostToDevice),
                    "cudaMemcpy(HtoD octree leaf indices)")) {
                return;
            }
        }

        updateParticlesOctree<<<(_particles.size() + BLOCK_SIZE - 1) / BLOCK_SIZE, BLOCK_SIZE>>>(
            last,
            d_particles,
            static_cast<int>(_particles.size()),
            _dOctreeNodes,
            rootIndex,
            _dOctreeLeafIndices,
            _octreeTheta,
            _octreeSoftening,
            deltaTime
        );
        if (!checkCudaStatus(cudaGetLastError(), "updateParticlesOctree kernel launch")) {
            return;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "updateParticlesOctree kernel sync")) {
            return;
        }

        if (!checkCudaStatus(
                cudaMemcpy(_particles.data(), d_particles, _particles.size() * sizeof(Particle), cudaMemcpyDeviceToHost),
                "cudaMemcpy(DtoH particles octree_gpu)")) {
            return;
        }
        applySphCorrection();
        applyThermalModel(deltaTime);
        return;
    }

#ifdef GRAVITY_PROFILE_LOGS
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);
#endif

    if (!d_particles || !last) {
        return;
    }

    const int numParticles = static_cast<int>(_particles.size());
    const int numBlocks = (numParticles + BLOCK_SIZE - 1) / BLOCK_SIZE;

    if (!checkCudaStatus(
            cudaMemcpy(last, _particles.data(), _particles.size() * sizeof(Particle), cudaMemcpyHostToDevice),
            "cudaMemcpy(HtoD base)")) {
        return;
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
        extractVelocityKernel<<<numBlocks, BLOCK_SIZE>>>(last, d_k1x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k1 launch")) {
            return;
        }
        computePairwiseAccelerationKernel<<<numBlocks, BLOCK_SIZE>>>(last, d_k1v, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "computeAcceleration k1 launch")) {
            return;
        }

        buildRk4StageKernel<<<numBlocks, BLOCK_SIZE>>>(last, d_k1x, d_k1v, 0.5f * deltaTime, d_stage, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k2 launch")) {
            return;
        }
        extractVelocityKernel<<<numBlocks, BLOCK_SIZE>>>(d_stage, d_k2x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k2 launch")) {
            return;
        }
        computePairwiseAccelerationKernel<<<numBlocks, BLOCK_SIZE>>>(d_stage, d_k2v, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "computeAcceleration k2 launch")) {
            return;
        }

        buildRk4StageKernel<<<numBlocks, BLOCK_SIZE>>>(last, d_k2x, d_k2v, 0.5f * deltaTime, d_stage, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k3 launch")) {
            return;
        }
        extractVelocityKernel<<<numBlocks, BLOCK_SIZE>>>(d_stage, d_k3x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k3 launch")) {
            return;
        }
        computePairwiseAccelerationKernel<<<numBlocks, BLOCK_SIZE>>>(d_stage, d_k3v, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "computeAcceleration k3 launch")) {
            return;
        }

        buildRk4StageKernel<<<numBlocks, BLOCK_SIZE>>>(last, d_k3x, d_k3v, deltaTime, d_stage, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k4 launch")) {
            return;
        }
        extractVelocityKernel<<<numBlocks, BLOCK_SIZE>>>(d_stage, d_k4x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k4 launch")) {
            return;
        }
        computePairwiseAccelerationKernel<<<numBlocks, BLOCK_SIZE>>>(d_stage, d_k4v, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "computeAcceleration k4 launch")) {
            return;
        }

        finalizeRk4Kernel<<<numBlocks, BLOCK_SIZE>>>(
            last,
            d_k1x, d_k2x, d_k3x, d_k4x,
            d_k1v, d_k2v, d_k3v, d_k4v,
            deltaTime,
            d_particles,
            numParticles
        );
        if (!checkCudaStatus(cudaGetLastError(), "finalizeRk4 launch")) {
            return;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "rk4 kernel sync")) {
            return;
        }
    } else {
        updateParticles<<<numBlocks, BLOCK_SIZE>>>(last, d_particles, numParticles, deltaTime);
        if (!checkCudaStatus(cudaGetLastError(), "updateParticles kernel launch")) {
            return;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "updateParticles kernel sync")) {
            return;
        }
    }

    if (!checkCudaStatus(
            cudaMemcpy(_particles.data(), d_particles, _particles.size() * sizeof(Particle), cudaMemcpyDeviceToHost),
            "cudaMemcpy(DtoH particles)")) {
        return;
    }

    applySphCorrection();
    applyThermalModel(deltaTime);

#ifdef GRAVITY_PROFILE_LOGS
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);
    printf("Time elapsed: %f ms (%f fps) for computing %zu particles\n", milliseconds, 1000.0f / milliseconds, _particles.size());
#endif
}


void destroyParticles(Particle *particles) {
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

    free(particles);
}

