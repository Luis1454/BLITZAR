#include "Internal.hpp"
SimulationServer::SimulationServer(std::uint32_t particleCount, float initialDt)
    : _running(false),
      _paused(false),
      _resetRequested(false),
      _cudaContextDirty(false),
      _stepRequests(0),
      _dt(initialDt),
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
      _snapshotTransferCap(resolvePublishedSnapshotCap(grav_protocol::kSnapshotDefaultPoints)),
      _lastAppliedSubstepTargetDt(0.0f),
      _lastAppliedSubstepDt(0.0f),
      _lastAppliedSubsteps(0u),
      _faulted(false),
      _faultStep(0),
      _particleCount(std::max<std::uint32_t>(2u, particleCount)),
      _solverMode("pairwise_cuda"),
      _integratorMode("euler"),
      _performanceProfile("interactive"),
      _octreeTheta(1.2f),
      _octreeSoftening(2.5f),
      _octreeOpeningCriterion("com"),
      _octreeEffectiveTheta(1.2f),
      _octreeThetaAutoMin(0.4f),
      _octreeThetaAutoMax(1.2f),
      _octreeDistributionScore(0.0f),
      _octreeThetaAutoTune(false),
      _sphEnabled(false),
      _sphSmoothingLength(1.25f),
      _sphRestDensity(1.0f),
      _sphGasConstant(4.0f),
      _sphViscosity(0.08f),
      _physicsMaxAcceleration(64.0f),
      _physicsMinSoftening(1e-4f),
      _physicsMinDistance2(1e-12f),
      _physicsMinTheta(0.05f),
      _sphMaxAcceleration(40.0f),
      _sphMaxSpeed(120.0f),
      _energyBaseline(0.0f),
      _hasEnergyBaseline(false),
      _exportDirectory("exports"),
      _exportFormatDefault("vtk"),
      _initialStatePath(),
      _initialStateFormat("auto"),
      _configPath("simulation.ini"),
      _runtimeConfigMirror(),
      _initialStateConfig(),
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
    _runtimeConfigMirror.particleCount = _particleCount;
    _runtimeConfigMirror.dt = std::max(1e-6f, initialDt);
    _runtimeConfigMirror.solver = _solverMode;
    _runtimeConfigMirror.integrator = _integratorMode;
    _runtimeConfigMirror.performanceProfile = _performanceProfile;
    _runtimeConfigMirror.substepTargetDt =
        _configuredSubstepTargetDt.load(std::memory_order_relaxed);
    _runtimeConfigMirror.maxSubsteps = _configuredMaxSubsteps.load(std::memory_order_relaxed);
    _runtimeConfigMirror.snapshotPublishPeriodMs =
        _snapshotPublishPeriodMs.load(std::memory_order_relaxed);
    _runtimeConfigMirror.clientParticleCap = grav_protocol::kSnapshotDefaultPoints;
    _runtimeConfigMirror.octreeTheta = _octreeTheta;
    _runtimeConfigMirror.octreeSoftening = _octreeSoftening;
    _runtimeConfigMirror.octreeOpeningCriterion = _octreeOpeningCriterion;
    _runtimeConfigMirror.octreeThetaAutoTune = _octreeThetaAutoTune;
    _runtimeConfigMirror.octreeThetaAutoMin = _octreeThetaAutoMin;
    _runtimeConfigMirror.octreeThetaAutoMax = _octreeThetaAutoMax;
    _runtimeConfigMirror.sphEnabled = _sphEnabled;
    _runtimeConfigMirror.sphSmoothingLength = _sphSmoothingLength;
    _runtimeConfigMirror.sphRestDensity = _sphRestDensity;
    _runtimeConfigMirror.sphGasConstant = _sphGasConstant;
    _runtimeConfigMirror.sphViscosity = _sphViscosity;
    _runtimeConfigMirror.physicsMaxAcceleration = _physicsMaxAcceleration;
    _runtimeConfigMirror.physicsMinSoftening = _physicsMinSoftening;
    _runtimeConfigMirror.physicsMinDistance2 = _physicsMinDistance2;
    _runtimeConfigMirror.physicsMinTheta = _physicsMinTheta;
    _runtimeConfigMirror.sphMaxAcceleration = _sphMaxAcceleration;
    _runtimeConfigMirror.sphMaxSpeed = _sphMaxSpeed;
    _runtimeConfigMirror.exportDirectory = _exportDirectory;
    _runtimeConfigMirror.exportFormat = _exportFormatDefault;
    _runtimeConfigMirror.inputFile = _initialStatePath;
    _runtimeConfigMirror.inputFormat = _initialStateFormat;
    _runtimeConfigMirror.energyMeasureEverySteps =
        _energyMeasureEverySteps.load(std::memory_order_relaxed);
    _runtimeConfigMirror.energySampleLimit = _energySampleLimit.load(std::memory_order_relaxed);
}
SimulationServer::SimulationServer(const std::string& configPath) : SimulationServer(2u, 0.01f)
{
    _configPath = configPath.empty() ? "simulation.ini" : configPath;
    SimulationConfig loaded = SimulationConfig::loadOrCreate(_configPath);
    // Apply simulation profile first, then performance profile
    grav_config::applySimulationProfile(loaded);
    grav_config::applyPerformanceProfile(loaded);
    const ResolvedInitialStatePlan initPlan = resolveInitialStatePlan(loaded, std::cerr);
    _runtimeConfigMirror = loaded;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _particleCount = std::max<std::uint32_t>(2u, loaded.particleCount);
        std::string solverCanonical;
        std::string integratorCanonical;
        _solverMode = grav_modes::normalizeSolver(loaded.solver, solverCanonical)
                          ? solverCanonical
                          : std::string(grav_modes::kSolverPairwiseCuda);
        _integratorMode = grav_modes::normalizeIntegrator(loaded.integrator, integratorCanonical)
                              ? integratorCanonical
                              : std::string(grav_modes::kIntegratorEuler);
        _performanceProfile = loaded.performanceProfile;
        coerceConfigSolverIntegratorCompatibility(_solverMode, _integratorMode, "config");
        _octreeTheta = loaded.octreeTheta;
        _octreeSoftening = loaded.octreeSoftening;
        _octreeOpeningCriterion = loaded.octreeOpeningCriterion;
        _octreeThetaAutoTune = loaded.octreeThetaAutoTune;
        _octreeThetaAutoMin = loaded.octreeThetaAutoMin;
        _octreeThetaAutoMax = loaded.octreeThetaAutoMax;
        _octreeEffectiveTheta = loaded.octreeTheta;
        _sphEnabled = loaded.sphEnabled;
        _sphSmoothingLength = loaded.sphSmoothingLength;
        _sphRestDensity = loaded.sphRestDensity;
        _sphGasConstant = loaded.sphGasConstant;
        _sphViscosity = loaded.sphViscosity;
        _physicsMaxAcceleration = loaded.physicsMaxAcceleration;
        _physicsMinSoftening = loaded.physicsMinSoftening;
        _physicsMinDistance2 = loaded.physicsMinDistance2;
        _physicsMinTheta = loaded.physicsMinTheta;
        _sphMaxAcceleration = loaded.sphMaxAcceleration;
        _sphMaxSpeed = loaded.sphMaxSpeed;
        _exportDirectory = loaded.exportDirectory;
        _exportFormatDefault = loaded.exportFormat;
        _initialStatePath = initPlan.inputFile;
        _initialStateFormat = initPlan.inputFormat;
        _initialStateConfig = initPlan.config;
    }
    std::cout << "[server] " << initPlan.summary << "\n";
    _dt.store(std::max(1e-6f, loaded.dt), std::memory_order_relaxed);
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
SimulationServer::~SimulationServer()
{
    stop();
    _system.reset();
}
void SimulationServer::start()
{
    if (_running.exchange(true)) {
        return;
    }
    startExportWorker();
    std::cout << "[server] start particles=" << _particleCount
              << " dt=" << _dt.load(std::memory_order_relaxed) << "\n";
    _thread = std::thread(&SimulationServer::loop, this);
}
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
void SimulationServer::setPaused(bool paused)
{
    _paused.store(paused, std::memory_order_relaxed);
}
bool SimulationServer::isPaused() const
{
    return _paused.load(std::memory_order_relaxed);
}
void SimulationServer::togglePaused()
{
    _paused.store(!_paused.load(std::memory_order_relaxed), std::memory_order_relaxed);
}
void SimulationServer::stepOnce()
{
    _stepRequests.fetch_add(1, std::memory_order_relaxed);
}
void SimulationServer::setDt(float dt)
{
    if (dt > 0.0f) {
        _dt.store(dt, std::memory_order_relaxed);
    }
}
void SimulationServer::scaleDt(float factor)
{
    if (factor <= 0.0f) {
        return;
    }
    const float current = _dt.load(std::memory_order_relaxed);
    setDt(current * factor);
}
float SimulationServer::getDt() const
{
    return _dt.load(std::memory_order_relaxed);
}
void SimulationServer::requestReset()
{
    _stepRequests.store(0, std::memory_order_relaxed);
    _serverFps.store(0.0f, std::memory_order_relaxed);
    _paused.store(false, std::memory_order_relaxed);
    clearPublishedSnapshotCache();
    _resetRequested.store(true, std::memory_order_relaxed);
}
void SimulationServer::requestRecover()
{
    requestReset();
}
void SimulationServer::setParticleCount(std::uint32_t particleCount)
{
    const std::uint32_t clamped = std::max<std::uint32_t>(2u, particleCount);
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _particleCount = clamped;
    }
    if (_running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}
