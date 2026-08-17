/*
 * @file engine/physics/cuda/integration/CudAdaptiveUpdate.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Advance the generic CUDA pairwise solver with dyadic adaptive time steps.
 */

/*
 * Module: cuda
 * Responsibility: Execute the native adaptive pairwise scheduler.
 */

bool ParticleSystem::updateCudaAdaptive(float deltaTime, const ForceLawPolicy& forceLaw,
                                        bool thermalActive, ParticleSoAView currentView,
                                        ParticleSoAView nextView, int numParticles, int numBlocks)
{
    if (_adaptiveTimeStepsEnabled && !_adaptiveTimeStepCostGuard &&
        _solverMode == SolverMode::PairwiseCuda) {
        const std::uint32_t levelCount = std::min<std::uint32_t>(_adaptiveTimeStepMaxLevel, 12u);
        const std::uint32_t sliceCount = 1u << levelCount;
        const float quantum = deltaTime / static_cast<float>(sliceCount);
        if (quantum <= 0.0f || !ensureAdaptiveCudaScratchCapacity(numParticles)) {
            fprintf(stderr, "[adaptive] CUDA scratch allocation failed\n");
            return false;
        }
        const bool resetSchedule =
            _adaptiveTimeStepTick == 0u || std::abs(_adaptiveTimeStepQuantum - quantum) > 1.0e-12f;
        if (resetSchedule) {
            if (!launchPairwiseAcceleration(currentView, _device->d_adaptiveAcceleration,
                                            numParticles, forceLaw)) {
                return false;
            }
            initializeAdaptiveScheduleKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, _device->d_adaptiveAcceleration, _device->d_adaptiveLevels,
                _device->d_adaptiveLastForceTicks, numParticles, static_cast<int>(levelCount),
                _adaptiveTimeStepEta, std::max(_octreeSoftening, _physicsMinSoftening), deltaTime);
            if (!checkCudaStatus(cudaGetLastError(), "adaptive pairwise schedule launch")) {
                return false;
            }
            _adaptiveTimeStepQuantum = quantum;
        }

        for (std::uint32_t slice = 0u; slice < sliceCount; ++slice) {
            const unsigned long long targetTick =
                static_cast<unsigned long long>(_adaptiveTimeStepTick + slice + 1u);
            adaptiveDriftKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                currentView, nextView, _device->d_adaptiveAcceleration, numParticles, quantum,
                _sphMaxSpeed);

            if (!checkCudaStatus(cudaGetLastError(), "adaptive pairwise drift launch")) {
                return false;
            }
            adaptivePairwiseCorrectKernel<<<numBlocks, Particle::kDefaultCudaBlockSize>>>(
                nextView, _device->d_adaptiveAcceleration, _device->d_adaptiveLevels,
                _device->d_adaptiveLastForceTicks, numParticles, forceLaw, _physicsMaxAcceleration,
                quantum, static_cast<int>(levelCount), _adaptiveTimeStepEta,
                std::max(_octreeSoftening, _physicsMinSoftening), deltaTime, targetTick,
                _sphMaxSpeed);
            if (!checkCudaStatus(cudaGetLastError(), "adaptive pairwise correction launch")) {
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
        if (!checkCudaStatus(cudaDeviceSynchronize(), "adaptive pairwise sync")) {
            return false;
        }
        _adaptiveTimeStepTick += sliceCount;
        _device->_hostStateDirty = true;
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
                    "[adaptive] backend=cuda_native solver=pairwise_cuda scheduler=dyadic "
                    "max_level=%u eta=%.4f\n",
                    levelCount, _adaptiveTimeStepEta);
            _adaptiveTimeStepMarkerPrinted = true;
        }
        publishMappedMetrics(deltaTime);
        return true;
    }

    return false;
}
