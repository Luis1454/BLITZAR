#include "server/simulation_server/Internal.hpp"
void SimulationServer::loop()
{
    rebuildSystem();
    auto nextSnapshotPublish = std::chrono::steady_clock::now();
    while (_running.load(std::memory_order_relaxed)) {
        if (_resetRequested.exchange(false, std::memory_order_relaxed)) {
            if (_cudaContextDirty.exchange(false, std::memory_order_relaxed)) {
                _system.reset();
                const cudaError_t resetStatus = cudaDeviceReset();
                if (resetStatus != cudaSuccess) {
                    std::cerr << "[server] cudaDeviceReset failed: "
                              << cudaGetErrorString(resetStatus) << "\n";
                }
                else {
                    std::cout << "[server] CUDA context reset after previous failure\n";
                }
            }
            rebuildSystem();
        }
        std::uint32_t stepBatch = 0;
        if (_paused.load(std::memory_order_relaxed)) {
            stepBatch = _stepRequests.exchange(0, std::memory_order_relaxed);
            if (stepBatch == 0) {
                processPendingExport();
                processPendingCheckpointSave();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
        }
        else {
            stepBatch = 1;
            _stepRequests.store(0, std::memory_order_relaxed);
        }
        float theta = 0.0f;
        float softening = 0.0f;
        float sphSmoothingLength = 0.0f;
        float sphRestDensity = 0.0f;
        float sphGasConstant = 0.0f;
        float sphViscosity = 0.0f;
        std::string solverMode;
        std::string integratorMode;
        std::string openingCriterion;
        bool sphEnabled = false;
        {
            std::lock_guard<std::mutex> lock(_commandMutex);
            theta = _octreeThetaAutoTune ? _octreeEffectiveTheta : _octreeTheta;
            softening = _octreeSoftening;
            solverMode = _solverMode;
            integratorMode = _integratorMode;
            openingCriterion = _octreeOpeningCriterion;
            sphEnabled = _sphEnabled;
            sphSmoothingLength = _sphSmoothingLength;
            sphRestDensity = _sphRestDensity;
            sphGasConstant = _sphGasConstant;
            sphViscosity = _sphViscosity;
        }
        _system->setSolverMode(solverModeFromCanonicalName(solverMode));
        _system->setIntegratorMode(integratorModeFromCanonicalName(integratorMode));
        _system->setOctreeTheta(theta);
        _system->setOctreeSoftening(softening);
        _system->setOctreeOpeningCriterion(openingCriterionFromCanonicalName(openingCriterion));
        _system->setSphEnabled(sphEnabled);
        _system->setSphParameters(sphSmoothingLength, sphRestDensity, sphGasConstant, sphViscosity);
        const auto batchStart = std::chrono::steady_clock::now();
        std::uint32_t executedSteps = 0;
        bool updateFailed = false;
        const bool steppedWhilePaused = _paused.load(std::memory_order_relaxed) && stepBatch > 0;
        for (std::uint32_t i = 0; i < stepBatch; ++i) {
            if (!_running.load(std::memory_order_relaxed) ||
                _resetRequested.load(std::memory_order_relaxed)) {
                break;
            }
            
            if (shouldForceCudaFailureOnceForTesting(solverMode)) {
                std::cerr << "[server] forcing CUDA failure once for integration test\n";
                updateFailed = true;
                break;
            }
            
            const float dt = std::max(1e-6f, _dt.load(std::memory_order_relaxed));
            const std::size_t liveParticleCount = _system ? _system->getParticles().size() : 0u;
            const bool eulerIntegrator =
                _system->getIntegratorMode() == ParticleSystem::IntegratorMode::Euler;
            const float configuredTargetSubstepDt =
                _configuredSubstepTargetDt.load(std::memory_order_relaxed);
            const float appliedTargetSubstepDt =
                configuredTargetSubstepDt > 0.0f
                    ? configuredTargetSubstepDt
                    : autoTargetSubstepDt(solverMode, eulerIntegrator, sphEnabled,
                                          liveParticleCount);
            const std::uint32_t configuredMaxSubsteps = std::max<std::uint32_t>(
                1u, _configuredMaxSubsteps.load(std::memory_order_relaxed));
            const std::uint32_t requiredSubsteps = std::max<std::uint32_t>(
                1u, static_cast<std::uint32_t>(std::ceil(dt / appliedTargetSubstepDt)));
            const std::uint32_t substeps =
                std::min<std::uint32_t>(requiredSubsteps, configuredMaxSubsteps);
            const float dtSub = dt / static_cast<float>(substeps);
            _lastAppliedSubstepTargetDt.store(appliedTargetSubstepDt,
                                              std::memory_order_relaxed);
            _lastAppliedSubstepDt.store(dtSub, std::memory_order_relaxed);
            _lastAppliedSubsteps.store(substeps, std::memory_order_relaxed);
            if (requiredSubsteps > configuredMaxSubsteps &&
                (_steps.load(std::memory_order_relaxed) % 256u) == 0u) {
                std::cerr << "[server] substep clamp active dt=" << dt
                          << " target_dt=" << appliedTargetSubstepDt
                          << " required=" << requiredSubsteps
                          << " max=" << configuredMaxSubsteps << " applied_dt=" << dtSub
                          << "\n";
            }
            const bool sampleGpuStep = _gpuTelemetryEnabled.load(std::memory_order_relaxed) &&
                                       ((solverMode == grav_modes::kSolverPairwiseCuda ||
                                         solverMode == grav_modes::kSolverOctreeGpu)) &&
                                       (((_steps.load(std::memory_order_relaxed) + 1u) %
                                         kGpuTelemetrySampleStride) == 0u);
            if (sampleGpuStep) {
                _gpuKernelMs.store(0.0f, std::memory_order_relaxed);
            }
            for (std::uint32_t s = 0; s < substeps; ++s) {
                const auto gpuStepStart = sampleGpuStep
                                              ? std::chrono::steady_clock::now()
                                              : std::chrono::steady_clock::time_point{};
                if (!_system->update(dtSub)) {
                    updateFailed = true;
                    break;
                }
                if (sampleGpuStep) {
                    const std::chrono::duration<float, std::milli> gpuStepElapsed =
                        std::chrono::steady_clock::now() - gpuStepStart;
                    _gpuKernelMs.store(gpuStepElapsed.count() +
                                           _gpuKernelMs.load(std::memory_order_relaxed),
                                       std::memory_order_relaxed);
                }
                atomicAddFloat(_totalTime, dtSub);
            }
            if (sampleGpuStep) {
                _gpuTelemetryAvailable.store(true, std::memory_order_relaxed);
            }
            else if (!_gpuTelemetryEnabled.load(std::memory_order_relaxed)) {
                clearGpuTelemetry();
            }
            if (updateFailed)
                break;
            _steps.fetch_add(1, std::memory_order_relaxed);
            ++executedSteps;
        }
        const auto batchEnd = std::chrono::steady_clock::now();
        const std::chrono::duration<float> elapsed = batchEnd - batchStart;
        if (executedSteps == 0 || elapsed.count() <= 1e-6f) {
            _serverFps.store(0.0f, std::memory_order_relaxed);
        }
        else {
            const float instantStepsPerSecond = static_cast<float>(executedSteps) / elapsed.count();
            const float previous = _serverFps.load(std::memory_order_relaxed);
            const float smoothed = (previous <= 0.0f)
                                       ? instantStepsPerSecond
                                       : (0.85f * previous + 0.15f * instantStepsPerSecond);
            _serverFps.store(smoothed, std::memory_order_relaxed);
        }
        if (updateFailed) {
            bool autoFallbackToCpu = false;
            std::string previousSolver;
            {
                std::lock_guard<std::mutex> lock(_commandMutex);
                previousSolver = _solverMode;
                if (_solverMode == "pairwise_cuda" || _solverMode == "octree_gpu")
                    _solverMode = "octree_cpu";
                autoFallbackToCpu = true;
            }
            if (autoFallbackToCpu) {
                _serverFps.store(0.0f, std::memory_order_relaxed);
                _cudaContextDirty.store(true, std::memory_order_relaxed);
                requestReset();
                _faulted.store(false, std::memory_order_relaxed);
                _faultStep.store(0, std::memory_order_relaxed);
                {
                    std::lock_guard<std::mutex> lock(_faultMutex);
                    _faultReason.clear();
                }
                std::cerr << "[server] CUDA update failed with solver=" << previousSolver
                          << ", auto-fallback to solver=octree_cpu\n";
            }
            else {
                _serverFps.store(0.0f, std::memory_order_relaxed);
                _paused.store(true, std::memory_order_relaxed);
                _cudaContextDirty.store(true, std::memory_order_relaxed);
                _faulted.store(true, std::memory_order_relaxed);
                _faultStep.store(_steps.load(std::memory_order_relaxed), std::memory_order_relaxed);
                {
                    std::lock_guard<std::mutex> lock(_faultMutex);
                    _faultReason = "cuda update failed (request recover/reset)";
                }
                std::cerr << "[server] update failed (CUDA error), simulation paused\n";
            }
        }
        const auto now = std::chrono::steady_clock::now();
        const bool publishByCadence = (executedSteps > 0) && (now >= nextSnapshotPublish);
        const bool publishAfterStepRequest = steppedWhilePaused && executedSteps > 0;
        if (publishByCadence || publishAfterStepRequest || updateFailed) {
            publishSnapshot();
            const auto snapshotPublishPeriod =
                std::chrono::milliseconds(_snapshotPublishPeriodMs.load(std::memory_order_relaxed));
            nextSnapshotPublish = now + snapshotPublishPeriod;
        }
        const std::uint64_t currentStep = _steps.load(std::memory_order_relaxed);
        maybeUpdateEnergy(currentStep);
        maybeSampleGpuTelemetry(solverMode, currentStep);
        processPendingExport();
        processPendingCheckpointSave();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
