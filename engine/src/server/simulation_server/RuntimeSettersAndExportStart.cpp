/*
 * @file engine/src/server/simulation_server/RuntimeSettersAndExportStart.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Source artifact for the BLITZAR simulation project.
 */

#include "Internal.hpp"

/*
 * @brief Documents the set solver mode operation contract.
 * @param mode Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setSolverMode(const std::string& mode)
{
    std::string canonical;
    if (!bltzr_modes::normalizeSolver(mode, canonical)) {
        std::cerr << "[server] ignored invalid solver mode: " << mode << "\n";
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        std::string nextSolver = canonical;
        if (!bltzr_modes::isSupportedSolverIntegratorPair(nextSolver, _integratorMode)) {
            std::cerr << "[server] rejected solver octree_gpu because integrator rk4 is not "
                         "supported with it\n";
            return;
        }
        if (_solverMode != nextSolver)
            _solverMode = nextSolver;
        changed = true;
        _runtimeConfigMirror.solver = _solverMode;
    }
    if (changed && _running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

/*
 * @brief Documents the set integrator mode operation contract.
 * @param mode Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setIntegratorMode(const std::string& mode)
{
    std::string canonical;
    if (!bltzr_modes::normalizeIntegrator(mode, canonical)) {
        std::cerr << "[server] ignored invalid integrator mode: " << mode << "\n";
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        std::string nextIntegrator = canonical;
        if (!bltzr_modes::isSupportedSolverIntegratorPair(_solverMode, nextIntegrator)) {
            std::cerr << "[server] rejected integrator rk4 because solver octree_gpu supports "
                         "euler only\n";
            return;
        }
        if (_integratorMode != nextIntegrator)
            _integratorMode = nextIntegrator;
        changed = true;
        _runtimeConfigMirror.integrator = _integratorMode;
    }
    if (changed && _running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

/*
 * @brief Documents the set performance profile operation contract.
 * @param profile Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setPerformanceProfile(const std::string& profile)
{
    std::string canonical;
    if (!bltzr_config::normalizePerformanceProfile(profile, canonical)) {
        std::cerr << "[server] ignored invalid performance profile: " << profile << "\n";
        return;
    }
    std::lock_guard<std::mutex> lock(_commandMutex);
    _performanceProfile = canonical;
    _runtimeConfigMirror.performanceProfile = canonical;
}

/*
 * @brief Documents the set octree parameters operation contract.
 * @param theta Input value used by this contract.
 * @param softening Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setOctreeParameters(float theta, float softening)
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    if (theta > 0.01f)
        _octreeTheta = theta;
    const float clampedMin = clampThetaBound(_octreeThetaAutoMin);
    const float clampedMax = std::max(clampedMin, clampThetaBound(_octreeThetaAutoMax));
    _octreeEffectiveTheta = std::clamp(theta, clampedMin, clampedMax);
    _runtimeConfigMirror.octreeTheta = _octreeTheta;
    if (softening > 1e-6f)
        _octreeSoftening = softening;
    _runtimeConfigMirror.octreeSoftening = _octreeSoftening;
}

/*
 * @brief Documents the set sph enabled operation contract.
 * @param enabled Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setSphEnabled(bool enabled)
{
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _sphEnabled = enabled;
    }
    if (_running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

/*
 * @brief Documents the set sph parameters operation contract.
 * @param smoothingLength Input value used by this contract.
 * @param restDensity Input value used by this contract.
 * @param gasConstant Input value used by this contract.
 * @param viscosity Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                                        float viscosity)
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    if (smoothingLength > 0.05f)
        _sphSmoothingLength = smoothingLength;
    if (restDensity > 0.01f)
        _sphRestDensity = restDensity;
    if (gasConstant > 0.01f)
        _sphGasConstant = gasConstant;
    if (viscosity >= 0.0f)
        _sphViscosity = viscosity;
}

/*
 * @brief Documents the set substep policy operation contract.
 * @param targetDt Input value used by this contract.
 * @param maxSubsteps Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps)
{
    const float safeTargetDt = std::max(0.0f, targetDt);
    const std::uint32_t safeMaxSubsteps = std::max<std::uint32_t>(1u, maxSubsteps);
    _configuredSubstepTargetDt.store(safeTargetDt, std::memory_order_relaxed);
    _configuredMaxSubsteps.store(safeMaxSubsteps, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(_commandMutex);
    _runtimeConfigMirror.substepTargetDt = safeTargetDt;
    _runtimeConfigMirror.maxSubsteps = safeMaxSubsteps;
}

/*
 * @brief Documents the set snapshot publish period ms operation contract.
 * @param periodMs Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setSnapshotPublishPeriodMs(std::uint32_t periodMs)
{
    const std::uint32_t safePeriodMs = std::max<std::uint32_t>(1u, periodMs);
    _snapshotPublishPeriodMs.store(safePeriodMs, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(_commandMutex);
    _runtimeConfigMirror.snapshotPublishPeriodMs = safePeriodMs;
}

/*
 * @brief Documents the set snapshot transfer cap operation contract.
 * @param maxPoints Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setSnapshotTransferCap(std::uint32_t maxPoints)
{
    const std::uint32_t safeMaxPoints = bltzr_protocol::clampSnapshotPoints(maxPoints);
    _snapshotTransferCap.store(resolvePublishedSnapshotCap(safeMaxPoints),
                               std::memory_order_relaxed);
    _runtimeConfigMirror.clientParticleCap = safeMaxPoints;
}

/*
 * @brief Documents the set initial state config operation contract.
 * @param config Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setInitialStateConfig(const InitialStateConfig& config)
{
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _initialStateConfig = config;
        _activeCheckpointState.reset();
    }
    if (_running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

/*
 * @brief Documents the set energy measurement config operation contract.
 * @param everySteps Input value used by this contract.
 * @param sampleLimit Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setEnergyMeasurementConfig(std::uint32_t everySteps,
                                                  std::uint32_t sampleLimit)
{
    const std::uint32_t safeEverySteps = std::max<std::uint32_t>(1u, everySteps);
    const std::uint32_t safeSampleLimit = std::max<std::uint32_t>(64u, sampleLimit);
    _energyMeasureEverySteps.store(safeEverySteps, std::memory_order_relaxed);
    _energySampleLimit.store(safeSampleLimit, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(_commandMutex);
    _runtimeConfigMirror.energyMeasureEverySteps = safeEverySteps;
    _runtimeConfigMirror.energySampleLimit = safeSampleLimit;
}

/*
 * @brief Documents the set gpu telemetry enabled operation contract.
 * @param enabled Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setGpuTelemetryEnabled(bool enabled)
{
    _gpuTelemetryEnabled.store(enabled, std::memory_order_relaxed);
    if (!enabled) {
        clearGpuTelemetry();
    }
}

/*
 * @brief Documents the set export defaults operation contract.
 * @param directory Input value used by this contract.
 * @param format Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setExportDefaults(const std::string& directory, const std::string& format)
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    if (!directory.empty()) {
        _exportDirectory = directory;
    }
    if (!format.empty()) {
        _exportFormatDefault = format;
    }
}

/*
 * @brief Documents the set initial state file operation contract.
 * @param path Input value used by this contract.
 * @param format Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setInitialStateFile(const std::string& path, const std::string& format)
{
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _initialStatePath = path;
        _initialStateFormat = format.empty() ? "auto" : format;
        _activeCheckpointState.reset();
        _runtimeConfigMirror.inputFile = _initialStatePath;
        _runtimeConfigMirror.inputFormat = _initialStateFormat;
        if (!_initialStatePath.empty()) {
            _initialStateConfig.mode = "file";
            _runtimeConfigMirror.initMode = "file";
            _runtimeConfigMirror.presetStructure = "file";
        }
    }
    if (_running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

/*
 * @brief Documents the request export snapshot operation contract.
 * @param outputPath Input value used by this contract.
 * @param format Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::requestExportSnapshot(const std::string& outputPath,
                                             const std::string& format)
{
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        PendingExportRequest request{};
        request.outputPath = outputPath;
        request.format = format.empty() ? _exportFormatDefault : format;
        _pendingExportRequests.push_back(std::move(request));
    }
    _exportQueueDepth.fetch_add(1u, std::memory_order_relaxed);
    updateExportStatus("queued", outputPath, "queued for background write");
}

/*
 * @brief Documents the start export worker operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::startExportWorker()
{
    if (_exportQueueState == nullptr || _exportQueueState->worker.joinable()) {
        return;
    }
    _exportQueueState->stopRequested = false;
    _exportQueueState->worker = std::thread([this]() {
        bool workerDone = false;
        while (!workerDone) {
            AsyncExportJob job{};
            {
                std::unique_lock<std::mutex> lock(_exportQueueState->mutex);
                _exportQueueState->condition.wait(lock, [this]() {
                    return _exportQueueState->stopRequested || !_exportQueueState->jobs.empty();
                });
                if (_exportQueueState->jobs.empty()) {
                    workerDone = _exportQueueState->stopRequested;
                    continue;
                }
                job = std::move(_exportQueueState->jobs.front());
                _exportQueueState->jobs.pop_front();
            }
            _exportActive.store(true, std::memory_order_relaxed);
            updateExportStatus("writing", job.outputPath, "writing snapshot on background worker");
            const bool ok = writeExportSnapshotFile(job);
            _exportActive.store(false, std::memory_order_relaxed);
            _exportQueueDepth.fetch_sub(1u, std::memory_order_relaxed);
            if (ok) {
                _exportCompletedCount.fetch_add(1u, std::memory_order_relaxed);
                updateExportStatus("completed", job.outputPath, "background export finished");
                std::cout << "[server] export ok: " << job.outputPath << "\n";
            }
            else {
                _exportFailedCount.fetch_add(1u, std::memory_order_relaxed);
                updateExportStatus("failed", job.outputPath, "background export failed");
                std::cerr << "[server] export failed: " << job.outputPath << "\n";
            }
        }
    });
}
