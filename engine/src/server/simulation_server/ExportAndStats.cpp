#include "Internal.hpp"
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
    result->condition.wait(waitLock, [result]() { return result->completed; });
    return result->ok;
}
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
        _particleCount = loaded.config.particleCount;
        _solverMode = loaded.config.solver;
        _integratorMode = loaded.config.integrator;
        _performanceProfile = loaded.config.performanceProfile;
        _octreeTheta = loaded.config.octreeTheta;
        _octreeSoftening = loaded.config.octreeSoftening;
        _octreeOpeningCriterion = loaded.config.octreeOpeningCriterion;
        _octreeThetaAutoTune = loaded.config.octreeThetaAutoTune;
        _octreeThetaAutoMin = loaded.config.octreeThetaAutoMin;
        _octreeThetaAutoMax = loaded.config.octreeThetaAutoMax;
        _sphEnabled = loaded.config.sphEnabled;
        _sphSmoothingLength = loaded.config.sphSmoothingLength;
        _sphRestDensity = loaded.config.sphRestDensity;
        _sphGasConstant = loaded.config.sphGasConstant;
        _sphViscosity = loaded.config.sphViscosity;
        _physicsMaxAcceleration = loaded.config.physicsMaxAcceleration;
        _physicsMinSoftening = loaded.config.physicsMinSoftening;
        _physicsMinDistance2 = loaded.config.physicsMinDistance2;
        _physicsMinTheta = loaded.config.physicsMinTheta;
        _sphMaxAcceleration = loaded.config.sphMaxAcceleration;
        _sphMaxSpeed = loaded.config.sphMaxSpeed;
        _runtimeConfigMirror.particleCount = loaded.config.particleCount;
        _runtimeConfigMirror.dt = loaded.config.dt;
        _runtimeConfigMirror.solver = loaded.config.solver;
        _runtimeConfigMirror.integrator = loaded.config.integrator;
        _runtimeConfigMirror.performanceProfile = loaded.config.performanceProfile;
        _runtimeConfigMirror.substepTargetDt = loaded.config.substepTargetDt;
        _runtimeConfigMirror.maxSubsteps = loaded.config.maxSubsteps;
        _runtimeConfigMirror.snapshotPublishPeriodMs = loaded.config.snapshotPublishPeriodMs;
        _runtimeConfigMirror.octreeTheta = loaded.config.octreeTheta;
        _runtimeConfigMirror.octreeSoftening = loaded.config.octreeSoftening;
        _runtimeConfigMirror.octreeOpeningCriterion = loaded.config.octreeOpeningCriterion;
        _runtimeConfigMirror.octreeThetaAutoTune = loaded.config.octreeThetaAutoTune;
        _runtimeConfigMirror.octreeThetaAutoMin = loaded.config.octreeThetaAutoMin;
        _runtimeConfigMirror.octreeThetaAutoMax = loaded.config.octreeThetaAutoMax;
        _runtimeConfigMirror.sphEnabled = loaded.config.sphEnabled;
        _runtimeConfigMirror.sphSmoothingLength = loaded.config.sphSmoothingLength;
        _runtimeConfigMirror.sphRestDensity = loaded.config.sphRestDensity;
        _runtimeConfigMirror.sphGasConstant = loaded.config.sphGasConstant;
        _runtimeConfigMirror.sphViscosity = loaded.config.sphViscosity;
        _runtimeConfigMirror.energyMeasureEverySteps = loaded.config.energyMeasureEverySteps;
        _runtimeConfigMirror.energySampleLimit = loaded.config.energySampleLimit;
        _runtimeConfigMirror.physicsMaxAcceleration = loaded.config.physicsMaxAcceleration;
        _runtimeConfigMirror.physicsMinSoftening = loaded.config.physicsMinSoftening;
        _runtimeConfigMirror.physicsMinDistance2 = loaded.config.physicsMinDistance2;
        _runtimeConfigMirror.physicsMinTheta = loaded.config.physicsMinTheta;
        _runtimeConfigMirror.sphMaxAcceleration = loaded.config.sphMaxAcceleration;
        _runtimeConfigMirror.sphMaxSpeed = loaded.config.sphMaxSpeed;
        _runtimeConfigMirror.inputFile = inputPath;
        _runtimeConfigMirror.inputFormat = "checkpoint";
        _runtimeConfigMirror.initMode = "file";
        _runtimeConfigMirror.presetStructure = "file";
        _dt.store(std::max(1e-6f, loaded.config.dt), std::memory_order_relaxed);
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
        _initialStatePath.clear();
        _initialStateFormat = "checkpoint";
        _activeCheckpointState.reset(new SimulationCheckpointState(loaded));
    }
    requestReset();
    return true;
}
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
void SimulationServer::updateExportStatus(const std::string& state, const std::string& path,
                                          const std::string& message)
{
    std::lock_guard<std::mutex> lock(_exportStatusMutex);
    _exportLastState = state;
    _exportLastPath = path;
    _exportLastMessage = message;
}
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
        mode = solverModeFromCanonicalName(_solverMode);
        integratorMode = _integratorMode;
        performanceProfile = _performanceProfile;
        particleCount = _particleCount;
        sphEnabled = _sphEnabled;
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
