/*
 * @file engine/src/server/simulation/lifecycle/Controls.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Source artifact for the BLITZAR simulation project.
 */

#include "engine/src/server/simulation/Internal.hpp"

/*
 * @brief Documents the simulation server operation contract.
 * @param particleCount Input value used by this contract.
 * @param initialDt Input value used by this contract.
 * @return SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
SimulationServer::SimulationServer(std::uint32_t particleCount, float initialDt)
    /*
     * @brief Documents the running operation contract.
     * @param false Input value used by this contract.
     * @param false Input value used by this contract.
     * @param false Input value used by this contract.
     * @param false Input value used by this contract.
     * @param _stepRequests Input value used by this contract.
     * @param initialDt Input value used by this contract.
     * @param _steps Input value used by this contract.
     * @param f Input value used by this contract.
     * @param f Input value used by this contract.
     * @param f Input value used by this contract.
     * @param f Input value used by this contract.
     * @param f Input value used by this contract.
     * @return : value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    : _running(false),
      _paused(false),
      _resetRequested(false),
      _cudaContextDirty(false),
      _stepRequests(0),
      _dt(clampSimulationDt(initialDt)),
      _steps(0),
      _serverFps(0.0f),
      _kineticEnergy(0.0f),
      _potentialEnergy(0.0f),
      _thermalEnergy(0.0f),
      _radiatedEnergy(0.0f),
      _totalEnergy(0.0f),
      _energyDriftPct(0.0f),
      _energyEstimated(false),
      _totalTime(0.0f),
      _energyMeasureEverySteps(120),
      _energySampleLimit(256),
      _gpuTelemetryEnabled(false),
      _gpuTelemetryAvailable(false),
      _gpuKernelMs(0.0f),
      _gpuCopyMs(0.0f),
      _gpuVramUsedBytes(0u),
      _gpuVramTotalBytes(0u),
      _exportQueueDepth(0u),
      _exportActive(false),
      _exportCompletedCount(0u),
      _exportFailedCount(0u),
      _configuredSubstepTargetDt(0.01f),
      _configuredMaxSubsteps(4u),
      _snapshotPublishPeriodMs(50u),
      _snapshotTransferCap(resolvePublishedSnapshotCap(bltzr_protocol::kSnapshotDefaultPoints)),
      _lastAppliedSubstepTargetDt(0.0f),
      _lastAppliedSubstepDt(0.0f),
      _lastAppliedSubsteps(0u),
      _faulted(false),
      _faultStep(0),
      _configState(std::max<std::uint32_t>(2u, particleCount)),
      _thread(),
      _snapshotMutex(),
      _commandMutex(),
      _faultMutex(),
      _exportStatusMutex(),
      _pendingExportRequests(),
      _publishedSnapshot(),
      _scratchSnapshot(),
      _faultReason(),
      _exportLastState("idle"),
      _exportLastPath(),
      _exportLastMessage(),
      _system(nullptr),
      _exportQueueState(new ExportQueueState()),
      _activeCheckpointState(nullptr),
      _checkpointQueueState(new SimulationCheckpointQueueState())
{
    _configState._runtimeConfigMirror.particleCount = _configState._particleCount;
    _configState._runtimeConfigMirror.dt = std::max(1e-6f, initialDt);
    _configState._runtimeConfigMirror.solver = _configState._solverMode;
    _configState._runtimeConfigMirror.integrator = _configState._integratorMode;
    _configState._runtimeConfigMirror.performanceProfile = _configState._performanceProfile;
    _configState._runtimeConfigMirror.substepTargetDt =
        _configuredSubstepTargetDt.load(std::memory_order_relaxed);
    _configState._runtimeConfigMirror.maxSubsteps = _configuredMaxSubsteps.load(std::memory_order_relaxed);
    _configState._runtimeConfigMirror.snapshotPublishPeriodMs =
        _snapshotPublishPeriodMs.load(std::memory_order_relaxed);
    _configState._runtimeConfigMirror.clientParticleCap = bltzr_protocol::kSnapshotDefaultPoints;
    _configState._runtimeConfigMirror.octreeTheta = _configState._octreeTheta;
    _configState._runtimeConfigMirror.octreeSoftening = _configState._octreeSoftening;
    _configState._runtimeConfigMirror.octreeOpeningCriterion = _configState._octreeOpeningCriterion;
    _configState._runtimeConfigMirror.octreeThetaAutoTune = _configState._octreeThetaAutoTune;
    _configState._runtimeConfigMirror.octreeThetaAutoMin = _configState._octreeThetaAutoMin;
    _configState._runtimeConfigMirror.octreeThetaAutoMax = _configState._octreeThetaAutoMax;
    _configState._runtimeConfigMirror.sphEnabled = _configState._sphEnabled;
    _configState._runtimeConfigMirror.sphSmoothingLength = _configState._sphSmoothingLength;
    _configState._runtimeConfigMirror.sphRestDensity = _configState._sphRestDensity;
    _configState._runtimeConfigMirror.sphGasConstant = _configState._sphGasConstant;
    _configState._runtimeConfigMirror.sphViscosity = _configState._sphViscosity;
    _configState._runtimeConfigMirror.physicsMaxAcceleration = _configState._physicsMaxAcceleration;
    _configState._runtimeConfigMirror.physicsMinSoftening = _configState._physicsMinSoftening;
    _configState._runtimeConfigMirror.physicsMinDistance2 = _configState._physicsMinDistance2;
    _configState._runtimeConfigMirror.physicsMinTheta = _configState._physicsMinTheta;
    _configState._runtimeConfigMirror.sphMaxAcceleration = _configState._sphMaxAcceleration;
    _configState._runtimeConfigMirror.sphMaxSpeed = _configState._sphMaxSpeed;
    _configState._runtimeConfigMirror.exportDirectory = _configState._exportDirectory;
    _configState._runtimeConfigMirror.exportFormat = _configState._exportFormatDefault;
    _configState._runtimeConfigMirror.inputFile = _configState._initialStatePath;
    _configState._runtimeConfigMirror.inputFormat = _configState._initialStateFormat;
    _configState._runtimeConfigMirror.energyMeasureEverySteps =
        _energyMeasureEverySteps.load(std::memory_order_relaxed);
    _configState._runtimeConfigMirror.energySampleLimit = _energySampleLimit.load(std::memory_order_relaxed);
}

/*
 * @brief Documents the simulation server operation contract.
 * @param f Input value used by this contract.
 * @return SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
SimulationServer::SimulationServer(const std::string& configPath) : SimulationServer(2u, 0.01f)
{
    _configState._configPath = configPath.empty() ? "simulation.ini" : configPath;
    SimulationConfig loaded = SimulationConfig::loadOrCreate(_configState._configPath);
    // Apply simulation profile first, then performance profile
    bltzr_config::applySimulationProfile(loaded);
    bltzr_config::applyPerformanceProfile(loaded);
    const ResolvedInitialStatePlan initPlan = resolveInitialStatePlan(loaded, std::cerr);
    _configState._runtimeConfigMirror = loaded;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _configState._particleCount = std::max<std::uint32_t>(2u, loaded.particleCount);
        std::string solverCanonical;
        std::string integratorCanonical;
        _configState._solverMode = bltzr_modes::normalizeSolver(loaded.solver, solverCanonical)
                          ? executableSolverMode(solverCanonical)
                          : std::string(bltzr_modes::kSolverPairwiseCuda);
        _configState._integratorMode = bltzr_modes::normalizeIntegrator(loaded.integrator, integratorCanonical)
                              ? integratorCanonical
                              : std::string(bltzr_modes::kIntegratorEuler);
        _configState._performanceProfile = loaded.performanceProfile;
        coerceConfigSolverIntegratorCompatibility(_configState._solverMode, _configState._integratorMode, "config");
        _configState._octreeTheta = loaded.octreeTheta;
        _configState._octreeSoftening = loaded.octreeSoftening;
        _configState._octreeOpeningCriterion = loaded.octreeOpeningCriterion;
        _configState._octreeThetaAutoTune = loaded.octreeThetaAutoTune;
        _configState._octreeThetaAutoMin = loaded.octreeThetaAutoMin;
        _configState._octreeThetaAutoMax = loaded.octreeThetaAutoMax;
        _configState._octreeEffectiveTheta = loaded.octreeTheta;
        _configState._sphEnabled = loaded.sphEnabled;
        _configState._sphSmoothingLength = loaded.sphSmoothingLength;
        _configState._sphRestDensity = loaded.sphRestDensity;
        _configState._sphGasConstant = loaded.sphGasConstant;
        _configState._sphViscosity = loaded.sphViscosity;
        _configState._physicsMaxAcceleration = loaded.physicsMaxAcceleration;
        _configState._physicsMinSoftening = loaded.physicsMinSoftening;
        _configState._physicsMinDistance2 = loaded.physicsMinDistance2;
        _configState._physicsMinTheta = loaded.physicsMinTheta;
        _configState._sphMaxAcceleration = loaded.sphMaxAcceleration;
        _configState._sphMaxSpeed = loaded.sphMaxSpeed;
        _configState._exportDirectory = loaded.exportDirectory;
        _configState._exportFormatDefault = loaded.exportFormat;
        _configState._initialStatePath = initPlan.inputFile;
        _configState._initialStateFormat = initPlan.inputFormat;
        _configState._initialStateConfig = initPlan.config;
    }
    std::cout << "[server] " << initPlan.summary << "\n";
    _dt.store(clampSimulationDt(loaded.dt), std::memory_order_relaxed);
    _energyMeasureEverySteps.store(std::max<std::uint32_t>(1u, loaded.energyMeasureEverySteps),
                                   std::memory_order_relaxed);
    _energySampleLimit.store(std::max<std::uint32_t>(64u, loaded.energySampleLimit),
                             std::memory_order_relaxed);
    _configuredSubstepTargetDt.store(std::max(0.0f, loaded.substepTargetDt),
                                     std::memory_order_relaxed);
    _configuredMaxSubsteps.store(std::max<std::uint32_t>(1u, loaded.maxSubsteps),
                                 std::memory_order_relaxed);
    _snapshotPublishPeriodMs.store(std::max<std::uint32_t>(1u, loaded.snapshotPublishPeriodMs),
                                   std::memory_order_relaxed);
    _snapshotTransferCap.store(resolvePublishedSnapshotCap(loaded.clientParticleCap),
                               std::memory_order_relaxed);
}

/*
 * @brief Documents the ~simulation server operation contract.
 * @param None This contract does not take explicit parameters.
 * @return SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
SimulationServer::~SimulationServer()
{
    stop();
    _system.reset();
}

/*
 * @brief Documents the start operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::start()
{
    if (_running.exchange(true)) {
        return;
    }
    startExportWorker();
    std::cout << "[server] start particles=" << _configState._particleCount
              << " dt=" << _dt.load(std::memory_order_relaxed) << "\n";
    _thread = std::thread(&SimulationServer::loop, this);
}

/*
 * @brief Documents the stop operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::stop()
{
    if (!_running.exchange(false)) {
        return;
    }
    std::cout << "[server] stop requested\n";
    if (_thread.joinable()) {
        _thread.join();
    }
    bool hasPendingRequests = true;
    while (hasPendingRequests) {
        {
            std::lock_guard<std::mutex> lock(_commandMutex);
            hasPendingRequests = !_pendingExportRequests.empty();
        }
        if (hasPendingRequests) {
            processPendingExport();
        }
    }
    stopExportWorker();
}

/*
 * @brief Documents the set paused operation contract.
 * @param paused Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setPaused(bool paused)
{
    _paused.store(paused, std::memory_order_relaxed);
}

/*
 * @brief Documents the is paused operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationServer::isPaused() const
{
    return _paused.load(std::memory_order_relaxed);
}

/*
 * @brief Documents the toggle paused operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::togglePaused()
{
    _paused.store(!_paused.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

/*
 * @brief Documents the step once operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::stepOnce()
{
    _stepRequests.fetch_add(1, std::memory_order_relaxed);
}

/*
 * @brief Documents the set dt operation contract.
 * @param dt Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setDt(float dt)
{
    _dt.store(clampSimulationDt(dt), std::memory_order_relaxed);
}

/*
 * @brief Documents the scale dt operation contract.
 * @param factor Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::scaleDt(float factor)
{
    if (factor <= 0.0f) {
        return;
    }
    const float current = _dt.load(std::memory_order_relaxed);
    setDt(current * factor);
}

/*
 * @brief Documents the get dt operation contract.
 * @param None This contract does not take explicit parameters.
 * @return float SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
float SimulationServer::getDt() const
{
    return _dt.load(std::memory_order_relaxed);
}

/*
 * @brief Documents the request reset operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::requestReset()
{
    _stepRequests.store(0, std::memory_order_relaxed);
    _serverFps.store(0.0f, std::memory_order_relaxed);
    _paused.store(false, std::memory_order_relaxed);
    clearPublishedSnapshotCache();
    _resetRequested.store(true, std::memory_order_relaxed);
}

/*
 * @brief Documents the request recover operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::requestRecover()
{
    requestReset();
}

/*
 * @brief Documents the set particle count operation contract.
 * @param particleCount Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setParticleCount(std::uint32_t particleCount)
{
    const std::uint32_t clamped = std::max<std::uint32_t>(2u, particleCount);
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _configState._particleCount = clamped;
    }
    if (_running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}
