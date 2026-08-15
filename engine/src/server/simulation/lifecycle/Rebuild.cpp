/*
 * @file engine/src/server/simulation/lifecycle/Rebuild.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Source artifact for the BLITZAR simulation project.
 */

#include "engine/src/server/simulation/Internal.hpp"

/*
 * @brief Documents the rebuild system operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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
    bool adaptiveTimeStepsEnabled = false;
    std::uint32_t adaptiveTimeStepMaxLevel = 4u;
    float adaptiveTimeStepEta = 0.25f;
    bool adaptiveTimeStepCostGuard = true;
    bool treePmEnabled = false;
    std::string treePmModel = "auto";
    std::string treePmLayout = "auto";
    std::string treePmPrecision = "fp32";
    std::string treePmAssignment = "cic";
    bool treePmLocalGrid = true;
    std::uint32_t treePmGridSize = 64u;
    std::uint32_t treePmJacobiIterations = 12u;
    float treePmCutoffFactor = 1.0f;
    std::uint32_t treePmMaxLocalNeighbors = 64u;
    std::uint32_t treePmParticleLimit = 0u;
    std::uint32_t treePmDenseCellThreshold = 64u;
    bool treePmGravityOnlyBuffers = true;
    float theta = 1.2f;
    float effectiveTheta = 1.2f;
    float softening = 2.5f;
    float thetaAutoMin = 0.4f;
    float thetaAutoMax = 1.2f;
    float octreeDistributionScore = 0.0f;
    bool thetaAutoTune = false;
    bool sphEnabled = false;
    bool deterministicMode = false;
    float sphSmoothingLength = 1.25f;
    float sphRestDensity = 1.0f;
    float sphGasConstant = 4.0f;
    float sphViscosity = 0.08f;
    float physicsMaxAcceleration = kPhysicsMaxAccelerationDefault;
    float physicsMinSoftening = kPhysicsMinSofteningDefault;
    float physicsMinDistance2 = kPhysicsMinDistance2Default;
    float physicsMinTheta = kPhysicsMinTheta;
    float sphMaxAcceleration = kSphMaxAccelerationDefault;
    float sphMaxSpeed = kSphMaxSpeedDefault;
    std::string inputPath;
    std::string inputFormat;
    InitialStateConfig initConfig;
    std::unique_ptr<SimulationCheckpointState> checkpointStateCopy;
    std::uint32_t configuredParticleCount = 2u;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        solver = _configState._solverMode;
        integrator = _configState._integratorMode;
        openingCriterion = _configState._octreeOpeningCriterion;
        performanceProfile = _configState._performanceProfile;
        adaptiveTimeStepsEnabled = _configState._adaptiveTimeStepsEnabled;
        adaptiveTimeStepMaxLevel = _configState._adaptiveTimeStepMaxLevel;
        adaptiveTimeStepEta = _configState._adaptiveTimeStepEta;
        adaptiveTimeStepCostGuard = _configState._runtimeConfigMirror.adaptiveTimeStepCostGuard;
        treePmEnabled = _configState._runtimeConfigMirror.treePmEnabled;
        treePmModel = _configState._runtimeConfigMirror.treePmModel;
        treePmLayout = _configState._runtimeConfigMirror.treePmLayout;
        treePmPrecision = _configState._runtimeConfigMirror.treePmPrecision;
        treePmAssignment = _configState._runtimeConfigMirror.treePmAssignment;
        treePmLocalGrid = _configState._runtimeConfigMirror.treePmLocalGrid;
        treePmGridSize = _configState._runtimeConfigMirror.treePmGridSize;
        treePmJacobiIterations = _configState._runtimeConfigMirror.treePmJacobiIterations;
        treePmCutoffFactor = _configState._runtimeConfigMirror.treePmCutoffFactor;
        treePmMaxLocalNeighbors = _configState._runtimeConfigMirror.treePmMaxLocalNeighbors;
        treePmParticleLimit = _configState._runtimeConfigMirror.treePmParticleLimit;
        treePmDenseCellThreshold = _configState._runtimeConfigMirror.treePmDenseCellThreshold;
        treePmGravityOnlyBuffers = _configState._runtimeConfigMirror.treePmGravityOnlyBuffers;
        theta = _configState._octreeTheta;
        effectiveTheta = _configState._octreeEffectiveTheta;
        softening = _configState._octreeSoftening;
        thetaAutoMin = _configState._octreeThetaAutoMin;
        thetaAutoMax = _configState._octreeThetaAutoMax;
        octreeDistributionScore = _configState._octreeDistributionScore;
        thetaAutoTune = _configState._octreeThetaAutoTune;
        sphEnabled = _configState._sphEnabled;
        deterministicMode = _configState._deterministicMode;
        sphSmoothingLength = _configState._sphSmoothingLength;
        sphRestDensity = _configState._sphRestDensity;
        sphGasConstant = _configState._sphGasConstant;
        sphViscosity = _configState._sphViscosity;
        physicsMaxAcceleration = _configState._physicsMaxAcceleration;
        physicsMinSoftening = _configState._physicsMinSoftening;
        physicsMinDistance2 = _configState._physicsMinDistance2;
        physicsMinTheta = _configState._physicsMinTheta;
        sphMaxAcceleration = _configState._sphMaxAcceleration;
        sphMaxSpeed = _configState._sphMaxSpeed;
        inputPath = _configState._initialStatePath;
        inputFormat = _configState._initialStateFormat;
        initConfig = _configState._initialStateConfig;
        if (_activeCheckpointState) {
            checkpointStateCopy =
                std::make_unique<SimulationCheckpointState>(*_activeCheckpointState);
        }
        configuredParticleCount = std::max<std::uint32_t>(2u, _configState._particleCount);
    }
    {
        std::string canonical;
        if (bltzr_modes::normalizeSolver(solver, canonical)) {
            solver = canonical;
        }
        else {
            solver.assign(bltzr_modes::kSolverPairwiseCuda);
            std::cerr
                << "[server] invalid internal solver mode detected, resetting to pairwise_cuda\n";
        }
        const std::string executableSolver = executableSolverMode(solver);
        if (executableSolver != solver) {
            std::cout << "[server] CUDA unavailable; using solver=" << executableSolver
                      << " instead of solver=" << solver << "\n";
            solver = executableSolver;
        }
        if (bltzr_modes::normalizeIntegrator(integrator, canonical)) {
            integrator = canonical;
        }
        else {
            integrator.assign(bltzr_modes::kIntegratorEuler);
            std::cerr << "[server] invalid internal integrator mode detected, resetting to euler\n";
        }
        if (bltzr_modes::normalizeOctreeOpeningCriterion(openingCriterion, canonical)) {
            openingCriterion = canonical;
        }
        else {
            openingCriterion.assign(bltzr_modes::kOctreeCriterionCom);
            std::cerr << "[server] invalid internal octree criterion detected, resetting to com\n";
        }
        coerceConfigSolverIntegratorCompatibility(solver, integrator, "rebuild");
    }
    std::vector<Particle> particles;
    const std::string initMode = toLower(initConfig.mode);
    const bool shouldTryFile = !checkpointStateCopy && initMode == "file" && !inputPath.empty();
    bool hasImportedState = false;
    if (checkpointStateCopy) {
        particles = std::move(checkpointStateCopy->particles);
        hasImportedState = particles.size() >= 2u;
    }
    else {
        hasImportedState = shouldTryFile && loadInitialState(particles, inputPath, inputFormat);
    }
    bool hasGeneratedState = false;
    if (!hasImportedState) {
        std::vector<Particle>().swap(particles);
        hasGeneratedState = buildGeneratedState(particles, configuredParticleCount, initConfig);
    }
    if (!checkpointStateCopy && !particles.empty()) {
        const std::size_t beforeTransform = particles.size();
        if (applyInitialStateTransform(particles, initConfig)) {
            std::cout << "[server] scene transform particles=" << beforeTransform << " -> "
                      << particles.size() << " copies=" << initConfig.sceneRotationCopies
                      << " mirror=" << (initConfig.sceneMirrorX || initConfig.sceneMirrorY ||
                                             initConfig.sceneMirrorZ ? "on" : "off")
                      << " offset=" << initConfig.sceneOffsetX << ',' << initConfig.sceneOffsetY
                      << ',' << initConfig.sceneOffsetZ << "\n";
        }
    }
    const bool hasInitialParticles = !particles.empty();
    const std::uint32_t targetParticleCount =
        hasInitialParticles
            ? static_cast<std::uint32_t>(std::max<std::size_t>(2u, particles.size()))
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
            effectiveSolver = executableSolverMode(std::string(bltzr_modes::kSolverOctreeGpu));
            std::cout << "[server] pairwise_cuda with " << targetParticleCount
                      << " particles is not realtime; auto-switching to " << effectiveSolver
                      << "\n";
        }
        else {
            std::cout << "[server] warning: pairwise_cuda with " << targetParticleCount
                      << " particles may look frozen; set solver=octree_gpu"
                      << " or BLITZAR_AUTO_SOLVER_FALLBACK=1\n";
        }
    }
    coerceConfigSolverIntegratorCompatibility(effectiveSolver, integrator, "rebuild");
    const ParticleSystem::SolverMode effectiveSolverMode =
        solverModeFromCanonicalName(effectiveSolver);
    const bool enableCudaRuntime =
        effectiveSolverMode == ParticleSystem::SolverMode::PairwiseCuda ||
        effectiveSolverMode == ParticleSystem::SolverMode::OctreeGpu || sphEnabled;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _configState._solverMode = effectiveSolver;
        _configState._integratorMode = integrator;
    }
    if (hasInitialParticles) {
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
        _system = std::make_unique<ParticleSystem>(std::move(particles), enableCudaRuntime);
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
        std::lock_guard<std::mutex> lock(_commandMutex);
        _configState._particleCount = targetParticleCount;
    }
    octreeDistributionScore = computeOctreeDistributionScore(configuredParticles);
    effectiveTheta =
        resolveOctreeTheta(theta, thetaAutoTune, thetaAutoMin, thetaAutoMax, performanceProfile,
                           configuredParticles, octreeDistributionScore);
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _configState._octreeOpeningCriterion = openingCriterion;
        _configState._octreeDistributionScore = octreeDistributionScore;
        _configState._octreeEffectiveTheta = effectiveTheta;
    }
    _system->setOctreeTheta(effectiveTheta);
    _system->setOctreeSoftening(softening);
    _system->setOctreeOpeningCriterion(openingCriterionFromCanonicalName(openingCriterion));
    _system->setSolverMode(effectiveSolverMode);
    _system->setIntegratorMode(integratorModeFromCanonicalName(integrator));
    _system->setSphEnabled(sphEnabled);
    _system->setDeterministicMode(deterministicMode);
    _system->setSphParameters(sphSmoothingLength, sphRestDensity, sphGasConstant, sphViscosity);
    _system->setPhysicsStabilityConstants(physicsMaxAcceleration, physicsMinSoftening,
                                          physicsMinDistance2, physicsMinTheta);
    _system->setSphCaps(sphMaxAcceleration, sphMaxSpeed);
    _system->setAdaptiveTimeStepParameters(adaptiveTimeStepsEnabled, adaptiveTimeStepMaxLevel,
                                           adaptiveTimeStepEta);
    _system->setAdaptiveTimeStepCostGuard(adaptiveTimeStepCostGuard);
    _system->setTreePmParameters(
        treePmEnabled, treePmModel, treePmLayout, treePmPrecision, treePmAssignment, treePmLocalGrid,
        static_cast<int>(treePmGridSize), static_cast<int>(treePmJacobiIterations),
        treePmCutoffFactor, static_cast<int>(treePmMaxLocalNeighbors),
        static_cast<int>(treePmParticleLimit), static_cast<int>(treePmDenseCellThreshold),
        treePmGravityOnlyBuffers);
    _system->setThermalParameters(initConfig.thermalAmbientTemperature,
                                  initConfig.thermalSpecificHeat, initConfig.thermalHeatingCoeff,
                                  initConfig.thermalRadiationCoeff);
    _system->setCosmologyParameters(initConfig.cosmology);
    std::cout << "[server] deterministic mode="
              << (_system->isDeterministicMode() ? "on" : "off") << "\n";
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
    _totalMass.store(0.0f, std::memory_order_relaxed);
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
    _configState._hasEnergyBaseline = false;
    _configState._energyBaseline = 0.0f;
    _faulted.store(false, std::memory_order_relaxed);
    _faultStep.store(0, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(_faultMutex);
        _faultReason.clear();
    }
    _scratchSnapshot.clear();
    publishSnapshot();
    maybeUpdateEnergy(0);
    if (checkpointStateCopy) {
        _steps.store(checkpointStateCopy->steps, std::memory_order_relaxed);
        _totalTime.store(checkpointStateCopy->totalTime, std::memory_order_relaxed);
        _paused.store(checkpointStateCopy->paused, std::memory_order_relaxed);
        _configState._hasEnergyBaseline = checkpointStateCopy->hasEnergyBaseline;
        _configState._energyBaseline = checkpointStateCopy->energyBaseline;
        maybeUpdateEnergy(checkpointStateCopy->steps);
    }
}
