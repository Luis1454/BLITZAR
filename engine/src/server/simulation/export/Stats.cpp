/*
 * @file engine/src/server/simulation/export/Stats.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Source artifact for the BLITZAR simulation project.
 */

#include "engine/src/server/simulation/Internal.hpp"

/*
 * @brief Documents the stop export worker operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::stopExportWorker()
{
    if (_exportQueueState == nullptr) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(_exportQueueState->mutex);
        _exportQueueState->stopRequested = true;
    }
    _exportQueueState->condition.notify_all();
    if (_exportQueueState->worker.joinable()) {
        _exportQueueState->worker.join();
    }
}

/*
 * @brief Documents the save checkpoint operation contract.
 * @param outputPath Input value used by this contract.
 * @return bool SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationServer::saveCheckpoint(const std::string& outputPath)
{
    if (outputPath.empty() || !_running.load(std::memory_order_relaxed) ||
        _checkpointQueueState == nullptr) {
        return false;
    }
    const std::shared_ptr<CheckpointSaveResult> result = std::make_shared<CheckpointSaveResult>();
    {
        std::lock_guard<std::mutex> lock(_checkpointQueueState->mutex);
        PendingCheckpointSaveRequest request{};
        request.outputPath = outputPath;
        request.result = result;
        _checkpointQueueState->saveRequests.push_back(std::move(request));
    }
    std::unique_lock<std::mutex> waitLock(result->mutex);
    result->condition.wait(waitLock, [result]() {
        return result->completed;
    });
    return result->ok;
}

/*
 * @brief Documents the load checkpoint operation contract.
 * @param inputPath Input value used by this contract.
 * @param outError Input value used by this contract.
 * @return bool SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationServer::loadCheckpoint(const std::string& inputPath, std::string* outError)
{
    if (inputPath.empty()) {
        if (outError != nullptr) {
            *outError = "missing checkpoint path";
        }
        return false;
    }
    SimulationCheckpointState loaded{};
    if (!readCheckpointFile(inputPath, loaded, outError)) {
        return false;
    }
    loaded.config.particleCount = static_cast<std::uint32_t>(loaded.particles.size());
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _configState._particleCount = loaded.config.particleCount;
        _configState._solverMode = loaded.config.solver;
        _configState._integratorMode = loaded.config.integrator;
        _configState._performanceProfile = loaded.config.performanceProfile;
        _configState._octreeTheta = loaded.config.octreeTheta;
        _configState._octreeSoftening = loaded.config.octreeSoftening;
        _configState._octreeOpeningCriterion = loaded.config.octreeOpeningCriterion;
        _configState._octreeThetaAutoTune = loaded.config.octreeThetaAutoTune;
        _configState._octreeThetaAutoMin = loaded.config.octreeThetaAutoMin;
        _configState._octreeThetaAutoMax = loaded.config.octreeThetaAutoMax;
        _configState._sphEnabled = loaded.config.sphEnabled;
        _configState._sphSmoothingLength = loaded.config.sphSmoothingLength;
        _configState._sphRestDensity = loaded.config.sphRestDensity;
        _configState._sphGasConstant = loaded.config.sphGasConstant;
        _configState._sphViscosity = loaded.config.sphViscosity;
        _configState._physicsMaxAcceleration = loaded.config.physicsMaxAcceleration;
        _configState._physicsMinSoftening = loaded.config.physicsMinSoftening;
        _configState._physicsMinDistance2 = loaded.config.physicsMinDistance2;
        _configState._physicsMinTheta = loaded.config.physicsMinTheta;
        _configState._sphMaxAcceleration = loaded.config.sphMaxAcceleration;
        _configState._sphMaxSpeed = loaded.config.sphMaxSpeed;
        _configState._runtimeConfigMirror.particleCount = loaded.config.particleCount;
        _configState._runtimeConfigMirror.dt = loaded.config.dt;
        _configState._runtimeConfigMirror.solver = loaded.config.solver;
        _configState._runtimeConfigMirror.integrator = loaded.config.integrator;
        _configState._runtimeConfigMirror.performanceProfile = loaded.config.performanceProfile;
        _configState._runtimeConfigMirror.substepTargetDt = loaded.config.substepTargetDt;
        _configState._runtimeConfigMirror.maxSubsteps = loaded.config.maxSubsteps;
        _configState._runtimeConfigMirror.snapshotPublishPeriodMs = loaded.config.snapshotPublishPeriodMs;
        _configState._runtimeConfigMirror.octreeTheta = loaded.config.octreeTheta;
        _configState._runtimeConfigMirror.octreeSoftening = loaded.config.octreeSoftening;
        _configState._runtimeConfigMirror.octreeOpeningCriterion = loaded.config.octreeOpeningCriterion;
        _configState._runtimeConfigMirror.octreeThetaAutoTune = loaded.config.octreeThetaAutoTune;
        _configState._runtimeConfigMirror.octreeThetaAutoMin = loaded.config.octreeThetaAutoMin;
        _configState._runtimeConfigMirror.octreeThetaAutoMax = loaded.config.octreeThetaAutoMax;
        _configState._runtimeConfigMirror.sphEnabled = loaded.config.sphEnabled;
        _configState._runtimeConfigMirror.sphSmoothingLength = loaded.config.sphSmoothingLength;
        _configState._runtimeConfigMirror.sphRestDensity = loaded.config.sphRestDensity;
        _configState._runtimeConfigMirror.sphGasConstant = loaded.config.sphGasConstant;
        _configState._runtimeConfigMirror.sphViscosity = loaded.config.sphViscosity;
        _configState._runtimeConfigMirror.energyMeasureEverySteps = loaded.config.energyMeasureEverySteps;
        _configState._runtimeConfigMirror.energySampleLimit = loaded.config.energySampleLimit;
        _configState._runtimeConfigMirror.physicsMaxAcceleration = loaded.config.physicsMaxAcceleration;
        _configState._runtimeConfigMirror.physicsMinSoftening = loaded.config.physicsMinSoftening;
        _configState._runtimeConfigMirror.physicsMinDistance2 = loaded.config.physicsMinDistance2;
        _configState._runtimeConfigMirror.physicsMinTheta = loaded.config.physicsMinTheta;
        _configState._runtimeConfigMirror.sphMaxAcceleration = loaded.config.sphMaxAcceleration;
        _configState._runtimeConfigMirror.sphMaxSpeed = loaded.config.sphMaxSpeed;
        _configState._runtimeConfigMirror.inputFile = inputPath;
        _configState._runtimeConfigMirror.inputFormat = "checkpoint";
        _configState._runtimeConfigMirror.initMode = "file";
        _configState._runtimeConfigMirror.presetStructure = "file";
        _dt.store(clampSimulationDt(loaded.config.dt), std::memory_order_relaxed);
        _configuredSubstepTargetDt.store(loaded.config.substepTargetDt, std::memory_order_relaxed);
        _configuredMaxSubsteps.store(std::max<std::uint32_t>(1u, loaded.config.maxSubsteps),
                                     std::memory_order_relaxed);
        _snapshotPublishPeriodMs.store(
            std::max<std::uint32_t>(1u, loaded.config.snapshotPublishPeriodMs),
            std::memory_order_relaxed);
        _energyMeasureEverySteps.store(
            std::max<std::uint32_t>(1u, loaded.config.energyMeasureEverySteps),
            std::memory_order_relaxed);
        _energySampleLimit.store(std::max<std::uint32_t>(2u, loaded.config.energySampleLimit),
                                 std::memory_order_relaxed);
        _gpuTelemetryEnabled.store(loaded.gpuTelemetryEnabled, std::memory_order_relaxed);
        _paused.store(loaded.paused, std::memory_order_relaxed);
        _configState._initialStatePath.clear();
        _configState._initialStateFormat = "checkpoint";
        _activeCheckpointState.reset(new SimulationCheckpointState(loaded));
    }
    requestReset();
    return true;
}

/*
 * @brief Documents the enqueue export write operation contract.
 * @param outputPath Input value used by this contract.
 * @param format Input value used by this contract.
 * @param particles Input value used by this contract.
 * @param solverModeLabel Input value used by this contract.
 * @param integratorModeLabel Input value used by this contract.
 * @param step Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::enqueueExportWrite(const std::string& outputPath, const std::string& format,
                                          const std::vector<Particle>& particles,
                                          const std::string& solverModeLabel,
                                          const std::string& integratorModeLabel,
                                          std::uint64_t step)
{
    AsyncExportJob job{};
    job.outputPath = outputPath;
    job.format = format;
    job.particles = particles;
    job.solverModeLabel = solverModeLabel;
    job.integratorModeLabel = integratorModeLabel;
    job.step = step;
    {
        std::lock_guard<std::mutex> lock(_exportQueueState->mutex);
        _exportQueueState->jobs.push_back(std::move(job));
    }
    _exportQueueState->condition.notify_one();
}

/*
 * @brief Documents the update export status operation contract.
 * @param state Input value used by this contract.
 * @param path Input value used by this contract.
 * @param message Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::updateExportStatus(const std::string& state, const std::string& path,
                                          const std::string& message)
{
    std::lock_guard<std::mutex> lock(_exportStatusMutex);
    _exportLastState = state;
    _exportLastPath = path;
    _exportLastMessage = message;
}

/*
 * @brief Documents the try consume snapshot operation contract.
 * @param outSnapshot Input value used by this contract.
 * @return bool SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationServer::tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot)
{
    std::lock_guard<std::mutex> lock(_snapshotMutex);
    if (_publishedSnapshot.empty()) {
        return false;
    }
    outSnapshot = std::move(_publishedSnapshot);
    _publishedSnapshot.clear();
    return true;
}

/*
 * @brief Documents the copy latest snapshot operation contract.
 * @param outSnapshot Input value used by this contract.
 * @param maxPoints Input value used by this contract.
 * @param outSourceSize Input value used by this contract.
 * @return bool SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationServer::copyLatestSnapshot(std::vector<RenderParticle>& outSnapshot,
                                          std::size_t maxPoints, std::size_t* outSourceSize) const
{
    std::lock_guard<std::mutex> lock(_snapshotMutex);
    if (_publishedSnapshot.empty()) {
        outSnapshot.clear();
        if (outSourceSize != nullptr) {
            *outSourceSize = 0u;
        }
        return false;
    }
    if (outSourceSize != nullptr) {
        *outSourceSize = _publishedSnapshot.size();
    }
    if (maxPoints == 0 || _publishedSnapshot.size() <= maxPoints) {
        outSnapshot = _publishedSnapshot;
        return true;
    }
    outSnapshot.clear();
    outSnapshot.reserve(maxPoints);
    const std::size_t stride =
        std::max<std::size_t>(1, (_publishedSnapshot.size() + maxPoints - 1u) / maxPoints);
    for (std::size_t i = 0; i < _publishedSnapshot.size() && outSnapshot.size() < maxPoints;
         i += stride) {
        outSnapshot.push_back(_publishedSnapshot[i]);
    }
    return true;
}

/*
 * @brief Documents the get stats operation contract.
 * @param None This contract does not take explicit parameters.
 * @return SimulationStats SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
SimulationStats SimulationServer::getStats() const
{
    ParticleSystem::SolverMode mode = ParticleSystem::SolverMode::PairwiseCuda;
    std::string integratorMode;
    std::string performanceProfile;
    std::uint32_t particleCount = 0u;
    bool sphEnabled = false;
    std::string exportLastState;
    std::string exportLastPath;
    std::string exportLastMessage;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        mode = solverModeFromCanonicalName(_configState._solverMode);
        integratorMode = _configState._integratorMode;
        performanceProfile = _configState._performanceProfile;
        particleCount = _configState._particleCount;
        sphEnabled = _configState._sphEnabled;
    }
    std::string faultReason;
    {
        std::lock_guard<std::mutex> lock(_faultMutex);
        faultReason = _faultReason;
    }
    {
        std::lock_guard<std::mutex> lock(_exportStatusMutex);
        exportLastState = _exportLastState;
        exportLastPath = _exportLastPath;
        exportLastMessage = _exportLastMessage;
    }
    return SimulationStats{_steps.load(std::memory_order_relaxed),
                           _dt.load(std::memory_order_relaxed),
                           _totalTime.load(std::memory_order_relaxed),
                           _paused.load(std::memory_order_relaxed),
                           _faulted.load(std::memory_order_relaxed),
                           _faultStep.load(std::memory_order_relaxed),
                           std::move(faultReason),
                           sphEnabled,
                           _serverFps.load(std::memory_order_relaxed),
                           std::move(performanceProfile),
                           _lastAppliedSubstepTargetDt.load(std::memory_order_relaxed),
                           _lastAppliedSubstepDt.load(std::memory_order_relaxed),
                           _lastAppliedSubsteps.load(std::memory_order_relaxed),
                           _configuredMaxSubsteps.load(std::memory_order_relaxed),
                           _snapshotPublishPeriodMs.load(std::memory_order_relaxed),
                           particleCount,
                           _kineticEnergy.load(std::memory_order_relaxed),
                           _potentialEnergy.load(std::memory_order_relaxed),
                           _thermalEnergy.load(std::memory_order_relaxed),
                           _radiatedEnergy.load(std::memory_order_relaxed),
                           _totalEnergy.load(std::memory_order_relaxed),
                           _energyDriftPct.load(std::memory_order_relaxed),
                           _energyEstimated.load(std::memory_order_relaxed),
                           std::string(solverLabel(mode)),
                           integratorMode,
                           _gpuTelemetryEnabled.load(std::memory_order_relaxed),
                           _gpuTelemetryAvailable.load(std::memory_order_relaxed),
                           _gpuKernelMs.load(std::memory_order_relaxed),
                           _gpuCopyMs.load(std::memory_order_relaxed),
                           _gpuVramUsedBytes.load(std::memory_order_relaxed),
                           _gpuVramTotalBytes.load(std::memory_order_relaxed),
                           _exportQueueDepth.load(std::memory_order_relaxed),
                           _exportActive.load(std::memory_order_relaxed),
                           _exportCompletedCount.load(std::memory_order_relaxed),
                           _exportFailedCount.load(std::memory_order_relaxed),
                           std::move(exportLastState),
                           std::move(exportLastPath),
                           std::move(exportLastMessage)};
}
