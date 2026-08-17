/*
 * @file engine/src/cuda/fragments/integration/CudaIntegrators.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Advance generic CUDA Euler, Leapfrog, and RK4 states.
 */

/*
 * Module: cuda
 * Responsibility: Execute standard CUDA integrator kernels.
 */

bool ParticleSystem::updateCudaIntegrators(float deltaTime, const ForceLawPolicy& forceLaw,
                                           bool thermalActive, ParticleSoAView currentView,
                                           ParticleSoAView nextView, int numParticles,
                                           int numBlocks)
{
    constexpr bool kProfileLogsEnabled = BLITZAR_PROFILE_LOGS != 0;
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    if constexpr (kProfileLogsEnabled) {
        cudaEventCreate(&start);
        cudaEventCreate(&stop);
        cudaEventRecord(start);
    }

    if (_integratorMode == IntegratorMode::Rk4 || _integratorMode == IntegratorMode::Leapfrog) {
        if (!_device->d_stage || !_device->d_k1x || !_device->d_k2x || !_device->d_k3x ||
            !_device->d_k4x || !_device->d_k1v || !_device->d_k2v || !_device->d_k3v ||
            !_device->d_k4v) {
            if (!allocateRk4Buffers(numParticles)) {
                fprintf(stderr, "[integrator] advanced integrator buffers missing\n");
                return false;
            }
        }
        if (_integratorMode == IntegratorMode::Leapfrog && !_device->d_vHalf) {
            if (!allocateRk4Buffers(numParticles)) {
                fprintf(stderr, "[integrator] leapfrog v_half buffer missing\n");
                return false;
            }
        }
    }

    if (_integratorMode == IntegratorMode::Rk4) {
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device->d_k1x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k1 launch")) {
            return false;
        }
        if (!launchPairwiseAcceleration(currentView, _device->d_k1v, numParticles, forceLaw)) {
            return false;
        }

        buildRk4StageKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device->d_k1x, _device->d_k1v, 0.5f * deltaTime, nextView, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k2 launch")) {
            return false;
        }
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            nextView, _device->d_k2x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k2 launch")) {
            return false;
        }
        if (!launchPairwiseAcceleration(nextView, _device->d_k2v, numParticles, forceLaw)) {
            return false;
        }

        buildRk4StageKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device->d_k2x, _device->d_k2v, 0.5f * deltaTime, nextView, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k3 launch")) {
            return false;
        }
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(

            nextView, _device->d_k3x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k3 launch")) {
            return false;
        }
        if (!launchPairwiseAcceleration(nextView, _device->d_k3v, numParticles, forceLaw)) {
            return false;
        }

        buildRk4StageKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device->d_k3x, _device->d_k3v, deltaTime, nextView, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "buildStage k4 launch")) {
            return false;
        }
        extractVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            nextView, _device->d_k4x, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "extractVelocity k4 launch")) {
            return false;
        }
        if (!launchPairwiseAcceleration(nextView, _device->d_k4v, numParticles, forceLaw)) {
            return false;
        }

        finalizeRk4Kernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device->d_k1x, _device->d_k2x, _device->d_k3x, _device->d_k4x, _device->d_k1v,
            _device->d_k2v, _device->d_k3v, _device->d_k4v, deltaTime, nextView, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "finalizeRk4 launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "rk4 kernel sync")) {
            return false;
        }
    }
    else if (_integratorMode == IntegratorMode::Leapfrog) {
        auto* halfVelocity = reinterpret_cast<float3*>(_device->d_vHalf.get());
        if (!_device->_leapfrogPrimed) {
            primeHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, halfVelocity, numParticles);
            if (!checkCudaStatus(cudaGetLastError(), "pairwise primeHalfVelocityKernel launch")) {
                return false;
            }
            _device->_leapfrogPrimed = true;
        }

        if (!launchPairwiseAcceleration(currentView, _device->d_k1v, numParticles, forceLaw)) {
            return false;
        }

        applyKickHalfStepKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, _device->d_k1v, deltaTime, halfVelocity, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "pairwise applyKickHalfStepKernel launch")) {
            return false;
        }

        driftWithHalfVelocityKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, halfVelocity, deltaTime, nextView, numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "pairwise driftWithHalfVelocityKernel launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "pairwise leapfrog drift sync")) {
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

        if (!launchPairwiseAcceleration(currentView, _device->d_k2v, numParticles, forceLaw)) {
            return false;
        }

        finalizeLeapfrogKickKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            currentView, halfVelocity, _device->d_k2v, deltaTime, nextView, halfVelocity,
            numParticles);
        if (!checkCudaStatus(cudaGetLastError(), "pairwise finalizeLeapfrogKickKernel launch")) {
            return false;
        }
        if (!checkCudaStatus(cudaDeviceSynchronize(), "pairwise leapfrog finalize sync")) {
            return false;
        }
    }
    else {
        _device->_leapfrogPrimed = false;
        if (_solverMode == SolverMode::PairwiseCuda) {
            if (!_device->d_k1v && !allocateRk4Buffers(numParticles)) {
                fprintf(stderr, "[pairwise] acceleration scratch allocation failed\n");
                return false;
            }
            if (!launchPairwiseAcceleration(currentView, _device->d_k1v, numParticles, forceLaw)) {
                return false;
            }
            updateParticlesWithAcceleration<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, nextView, _device->d_k1v, numParticles, deltaTime);
        }
        else {
            updateParticles<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, nextView, numParticles, deltaTime, forceLaw, _physicsMaxAcceleration);
        }
        if (!checkCudaStatus(cudaGetLastError(), "updateParticles kernel launch")) {
            return false;
        }

        if (!checkCudaStatus(cudaDeviceSynchronize(), "updateParticles kernel sync")) {
            return false;
        }
    }

    // Swap buffers
    std::swap(_device->d_soaPosX, _device->d_soaNextPosX);
    std::swap(_device->d_soaPosY, _device->d_soaNextPosY);
    std::swap(_device->d_soaPosZ, _device->d_soaNextPosZ);
    std::swap(_device->d_soaVelX, _device->d_soaNextVelX);
    std::swap(_device->d_soaVelY, _device->d_soaNextVelY);
    std::swap(_device->d_soaVelZ, _device->d_soaNextVelZ);

    float scaleRatio = 1.0f;
    float previousHubble = 0.0f;
    float nextHubble = 0.0f;
    if (prepareCosmologyStep(deltaTime, scaleRatio, previousHubble, nextHubble)) {
        applyCosmologyExpansionKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
            getSoAView(false), numParticles, scaleRatio, previousHubble, nextHubble);
        if (!checkCudaStatus(cudaGetLastError(), "cosmology expansion kernel launch") ||
            !checkCudaStatus(cudaDeviceSynchronize(), "cosmology expansion kernel sync")) {
            return false;
        }
    }

    if (!this->applySphCorrection(deltaTime, false)) {
        return false;
    }
    _device->_hostStateDirty = true;
    if (thermalActive) {
        if (!syncHostState()) {
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
        printf("Time elapsed: %f ms (%f fps) for computing %zu particles\n", milliseconds,
               1000.0f / milliseconds, _particles.size());
    }
    publishMappedMetrics(deltaTime);
    return true;
}
