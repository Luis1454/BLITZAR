// File: engine/src/server/simulation_server/RebuildSystem.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "Internal.hpp"
/// Description: Executes the rebuildSystem operation.
void SimulationServer::rebuildSystem()
{
    // Important with the current CUDA global-buffer model:
    // destroy the previous ParticleSystem before constructing the next one,
    // otherwise the old destructor may free buffers allocated by the new instance.
    _system.reset();
    std::string solver;
    std::string integrator;
    std::string openingCriterion = "com";
    std::string performanceProfile = "interactive";
    float theta = 1.2f;
    float effectiveTheta = 1.2f;
    float softening = 2.5f;
    float thetaAutoMin = 0.4f;
    float thetaAutoMax = 1.2f;
    float octreeDistributionScore = 0.0f;
    bool thetaAutoTune = false;
    bool sphEnabled = false;
    float sphSmoothingLength = 1.25f;
    float sphRestDensity = 1.0f;
    float sphGasConstant = 4.0f;
    float sphViscosity = 0.08f;
    float physicsMaxAcceleration = 64.0f;
    float physicsMinSoftening = 1e-4f;
    float physicsMinDistance2 = 1e-12f;
    float physicsMinTheta = 0.05f;
    float sphMaxAcceleration = 40.0f;
    float sphMaxSpeed = 120.0f;
    std::string inputPath;
    std::string inputFormat;
    InitialStateConfig initConfig;
    std::unique_ptr<SimulationCheckpointState> checkpointStateCopy;
    std::uint32_t configuredParticleCount = 2u;
    {
        /// Description: Executes the lock operation.
        std::lock_guard<std::mutex> lock(_commandMutex);
        solver = _solverMode;
        integrator = _integratorMode;
        openingCriterion = _octreeOpeningCriterion;
        performanceProfile = _performanceProfile;
        theta = _octreeTheta;
        effectiveTheta = _octreeEffectiveTheta;
        softening = _octreeSoftening;
        thetaAutoMin = _octreeThetaAutoMin;
        thetaAutoMax = _octreeThetaAutoMax;
        octreeDistributionScore = _octreeDistributionScore;
        thetaAutoTune = _octreeThetaAutoTune;
        sphEnabled = _sphEnabled;
        sphSmoothingLength = _sphSmoothingLength;
        sphRestDensity = _sphRestDensity;
        sphGasConstant = _sphGasConstant;
        sphViscosity = _sphViscosity;
        physicsMaxAcceleration = _physicsMaxAcceleration;
        physicsMinSoftening = _physicsMinSoftening;
        physicsMinDistance2 = _physicsMinDistance2;
        physicsMinTheta = _physicsMinTheta;
        sphMaxAcceleration = _sphMaxAcceleration;
        sphMaxSpeed = _sphMaxSpeed;
        inputPath = _initialStatePath;
        inputFormat = _initialStateFormat;
        initConfig = _initialStateConfig;
        if (_activeCheckpointState) {
            checkpointStateCopy.reset(new SimulationCheckpointState(*_activeCheckpointState));
        }
        configuredParticleCount = std::max<std::uint32_t>(2u, _particleCount);
    }
    {
        std::string canonical;
        if (grav_modes::normalizeSolver(solver, canonical)) {
            solver = canonical;
        }
        else {
            solver.assign(grav_modes::kSolverPairwiseCuda);
            std::cerr
                << "[server] invalid internal solver mode detected, resetting to pairwise_cuda\n";
        }
        if (grav_modes::normalizeIntegrator(integrator, canonical)) {
            integrator = canonical;
        }
        else {
            integrator.assign(grav_modes::kIntegratorEuler);
            std::cerr << "[server] invalid internal integrator mode detected, resetting to euler\n";
        }
        if (grav_modes::normalizeOctreeOpeningCriterion(openingCriterion, canonical)) {
            openingCriterion = canonical;
        }
        else {
            openingCriterion.assign(grav_modes::kOctreeCriterionCom);
            std::cerr << "[server] invalid internal octree criterion detected, resetting to com\n";
        }
        /// Description: Executes the coerceConfigSolverIntegratorCompatibility operation.
        coerceConfigSolverIntegratorCompatibility(solver, integrator, "rebuild");
    }
    std::vector<Particle> importedParticles;
    const std::string initMode = toLower(initConfig.mode);
    const bool shouldTryFile = !checkpointStateCopy && initMode == "file" && !inputPath.empty();
    bool hasImportedState = false;
    if (checkpointStateCopy) {
        importedParticles = checkpointStateCopy->particles;
        hasImportedState = importedParticles.size() >= 2u;
    }
    else {
        hasImportedState =
            /// Description: Executes the loadInitialState operation.
            shouldTryFile && loadInitialState(importedParticles, inputPath, inputFormat);
    }
    std::vector<Particle> generatedParticles;
    const bool hasGeneratedState =
        hasImportedState
            ? false
            : buildGeneratedState(generatedParticles, configuredParticleCount, initConfig);
    const std::vector<Particle>& initialParticles =
        hasImportedState ? importedParticles : generatedParticles;
    const bool hasInitialParticles = !initialParticles.empty();
    const std::uint32_t targetParticleCount =
        hasInitialParticles
            ? static_cast<std::uint32_t>(std::max<std::size_t>(2u, initialParticles.size()))
            : configuredParticleCount;
    std::cout << "[server] init rebuild mode=" << initMode
              << " active_source=" << (hasImportedState ? "file" : "generated");
    if (shouldTryFile) {
        std::cout << " input_file=" << inputPath
                  << " input_format=" << (inputFormat.empty() ? "auto" : inputFormat);
    }
    std::cout << " particles=" << targetParticleCount << "\n";
    std::string effectiveSolver = solver;
    if (solverModeFromCanonicalName(solver) == ParticleSystem::SolverMode::PairwiseCuda &&
        targetParticleCount > kPairwiseRealtimeParticleLimit) {
        if (isAutoSolverFallbackEnabled()) {
            effectiveSolver = "octree_gpu";
            std::cout << "[server] pairwise_cuda with " << targetParticleCount
                      << " particles is not realtime; auto-switching to octree_gpu\n";
        }
        else {
            std::cout << "[server] warning: pairwise_cuda with " << targetParticleCount
                      << " particles may look frozen; set solver=octree_gpu"
                      << " or GRAVITY_AUTO_SOLVER_FALLBACK=1\n";
        }
    }
    /// Description: Executes the coerceConfigSolverIntegratorCompatibility operation.
    coerceConfigSolverIntegratorCompatibility(effectiveSolver, integrator, "rebuild");
    {
        /// Description: Executes the lock operation.
        std::lock_guard<std::mutex> lock(_commandMutex);
        _solverMode = effectiveSolver;
        _integratorMode = integrator;
    }
    if (hasInitialParticles) {
        std::vector<Particle> particles = initialParticles;
        for (Particle& p : particles) {
            p.setPressure(Vector3(0.0f, 0.0f, 0.0f));
            p.setDensity(0.0f);
            float temp = p.getTemperature();
            if (temp < 0.0f) {
                temp = 0.0f;
            }
            if (temp == 0.0f && initConfig.particleTemperature > 0.0f) {
                temp = initConfig.particleTemperature;
            }
            p.setTemperature(temp);
        }
        _system = std::make_unique<ParticleSystem>(std::move(particles));
    }
    else {
        _system = std::make_unique<ParticleSystem>(static_cast<int>(targetParticleCount), false);
    }
    const std::vector<Particle>& configuredParticles = _system->getParticles();
    if (hasImportedState) {
        std::cout << "[server] loaded initial state from " << inputPath
                  << " particles=" << configuredParticles.size() << "\n";
    }
    else if (hasGeneratedState) {
        std::cout << "[server] generated initial state mode=" << initConfig.mode
                  << " particles=" << configuredParticles.size() << "\n";
    }
    else {
        std::cerr << "[server] initial state generation failed, using constructor fallback\n";
    }
    {
        /// Description: Executes the lock operation.
        std::lock_guard<std::mutex> lock(_commandMutex);
        _particleCount = targetParticleCount;
    }
    octreeDistributionScore = computeOctreeDistributionScore(configuredParticles);
    effectiveTheta =
        resolveOctreeTheta(theta, thetaAutoTune, thetaAutoMin, thetaAutoMax, performanceProfile,
                           configuredParticles, octreeDistributionScore);
    {
        /// Description: Executes the lock operation.
        std::lock_guard<std::mutex> lock(_commandMutex);
        _octreeOpeningCriterion = openingCriterion;
        _octreeDistributionScore = octreeDistributionScore;
        _octreeEffectiveTheta = effectiveTheta;
    }
    _system->setOctreeTheta(effectiveTheta);
    _system->setOctreeSoftening(softening);
    _system->setOctreeOpeningCriterion(openingCriterionFromCanonicalName(openingCriterion));
    _system->setSolverMode(solverModeFromCanonicalName(effectiveSolver));
    _system->setIntegratorMode(integratorModeFromCanonicalName(integrator));
    _system->setSphEnabled(sphEnabled);
    _system->setSphParameters(sphSmoothingLength, sphRestDensity, sphGasConstant, sphViscosity);
    _system->setPhysicsStabilityConstants(physicsMaxAcceleration, physicsMinSoftening,
                                          physicsMinDistance2, physicsMinTheta);
    _system->setSphCaps(sphMaxAcceleration, sphMaxSpeed);
    _system->setThermalParameters(initConfig.thermalAmbientTemperature,
                                  initConfig.thermalSpecificHeat, initConfig.thermalHeatingCoeff,
                                  initConfig.thermalRadiationCoeff);
    logEffectiveExecutionModes(
        effectiveSolver, integrator, performanceProfile, openingCriterion, theta, effectiveTheta,
        thetaAutoTune, thetaAutoMin, thetaAutoMax, octreeDistributionScore, softening,
        physicsMaxAcceleration, physicsMinSoftening, physicsMinDistance2, physicsMinTheta,
        sphEnabled, _configuredSubstepTargetDt.load(std::memory_order_relaxed),
        _configuredMaxSubsteps.load(std::memory_order_relaxed),
        _snapshotPublishPeriodMs.load(std::memory_order_relaxed),
        _serverFps.load(std::memory_order_relaxed),
        _energyDriftPct.load(std::memory_order_relaxed));
    if (initConfig.thermalHeatingCoeff > 0.0f || initConfig.thermalRadiationCoeff > 0.0f) {
        std::cout << "[server] warning: thermal model active (heating="
                  << initConfig.thermalHeatingCoeff
                  << ", radiation=" << initConfig.thermalRadiationCoeff
                  << ") may change total energy drift\n";
    }
    _steps.store(0, std::memory_order_relaxed);
    _serverFps.store(0.0f, std::memory_order_relaxed);
    _stepRequests.store(0, std::memory_order_relaxed);
    _kineticEnergy.store(0.0f, std::memory_order_relaxed);
    _potentialEnergy.store(0.0f, std::memory_order_relaxed);
    _thermalEnergy.store(0.0f, std::memory_order_relaxed);
    _radiatedEnergy.store(0.0f, std::memory_order_relaxed);
    _totalEnergy.store(0.0f, std::memory_order_relaxed);
    _energyDriftPct.store(0.0f, std::memory_order_relaxed);
    _energyEstimated.store(false, std::memory_order_relaxed);
    _totalTime.store(0.0f, std::memory_order_relaxed);
    _lastAppliedSubstepTargetDt.store(0.0f, std::memory_order_relaxed);
    _lastAppliedSubstepDt.store(0.0f, std::memory_order_relaxed);
    _lastAppliedSubsteps.store(0u, std::memory_order_relaxed);
    _hasEnergyBaseline = false;
    _energyBaseline = 0.0f;
    _faulted.store(false, std::memory_order_relaxed);
    _faultStep.store(0, std::memory_order_relaxed);
    {
        /// Description: Executes the lock operation.
        std::lock_guard<std::mutex> lock(_faultMutex);
        _faultReason.clear();
    }
    _scratchSnapshot.clear();
    /// Description: Executes the publishSnapshot operation.
    publishSnapshot();
    /// Description: Executes the maybeUpdateEnergy operation.
    maybeUpdateEnergy(0);
    if (checkpointStateCopy) {
        _steps.store(checkpointStateCopy->steps, std::memory_order_relaxed);
        _totalTime.store(checkpointStateCopy->totalTime, std::memory_order_relaxed);
        _paused.store(checkpointStateCopy->paused, std::memory_order_relaxed);
        _hasEnergyBaseline = checkpointStateCopy->hasEnergyBaseline;
        _energyBaseline = checkpointStateCopy->energyBaseline;
        /// Description: Executes the maybeUpdateEnergy operation.
        maybeUpdateEnergy(checkpointStateCopy->steps);
    }
}
