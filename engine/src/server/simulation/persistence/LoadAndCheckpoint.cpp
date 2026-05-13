/*
 * @file engine/src/server/simulation/persistence/LoadAndCheckpoint.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Source artifact for the BLITZAR simulation project.
 */

#include "engine/src/server/simulation/Internal.hpp"

/*
 * @brief Documents the get runtime config operation contract.
 * @param None This contract does not take explicit parameters.
 * @return SimulationConfig SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
SimulationConfig SimulationServer::getRuntimeConfig() const
{
    SimulationConfig config;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        config = _configState._runtimeConfigMirror;
        config.particleCount = _configState._particleCount;
        config.solver = _configState._solverMode;
        config.integrator = _configState._integratorMode;
        config.performanceProfile = _configState._performanceProfile;
        config.octreeTheta = _configState._octreeTheta;
        config.octreeSoftening = _configState._octreeSoftening;
        config.octreeOpeningCriterion = _configState._octreeOpeningCriterion;
        config.octreeThetaAutoTune = _configState._octreeThetaAutoTune;
        config.octreeThetaAutoMin = _configState._octreeThetaAutoMin;
        config.octreeThetaAutoMax = _configState._octreeThetaAutoMax;
        config.sphEnabled = _configState._sphEnabled;
        config.sphSmoothingLength = _configState._sphSmoothingLength;
        config.sphRestDensity = _configState._sphRestDensity;
        config.sphGasConstant = _configState._sphGasConstant;
        config.sphViscosity = _configState._sphViscosity;
        config.exportDirectory = _configState._exportDirectory;
        config.exportFormat = _configState._exportFormatDefault;
        config.inputFile = _configState._initialStatePath;
        config.inputFormat = _configState._initialStateFormat;
    }
    config.dt = _dt.load(std::memory_order_relaxed);
    config.substepTargetDt = _configuredSubstepTargetDt.load(std::memory_order_relaxed);
    config.maxSubsteps = _configuredMaxSubsteps.load(std::memory_order_relaxed);
    config.snapshotPublishPeriodMs = _snapshotPublishPeriodMs.load(std::memory_order_relaxed);
    config.energyMeasureEverySteps = _energyMeasureEverySteps.load(std::memory_order_relaxed);
    config.energySampleLimit = _energySampleLimit.load(std::memory_order_relaxed);
    config.physicsMaxAcceleration = _configState._physicsMaxAcceleration;
    config.physicsMinSoftening = _configState._physicsMinSoftening;
    config.physicsMinDistance2 = _configState._physicsMinDistance2;
    config.physicsMinTheta = _configState._physicsMinTheta;
    config.sphMaxAcceleration = _configState._sphMaxAcceleration;
    config.sphMaxSpeed = _configState._sphMaxSpeed;
    return config;
}

/*
 * @brief Documents the load initial state operation contract.
 * @param outParticles Input value used by this contract.
 * @param inputPath Input value used by this contract.
 * @param format Input value used by this contract.
 * @return bool SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationServer::loadInitialState(std::vector<Particle>& outParticles,
                                        const std::string& inputPath,
                                        const std::string& format) const
{
    outParticles.clear();
    if (inputPath.empty()) {
        return false;
    }
    const std::filesystem::path path(inputPath);
    if (!std::filesystem::exists(path)) {
        std::cerr << "[server] input file not found: " << inputPath << "\n";
        return false;
    }
    std::string fmt = normalizeSnapshotFormat(format.empty() ? std::string("auto") : format);
    if (fmt == "auto")
        fmt = normalizeSnapshotFormat(guessFormatFromPath(inputPath));
    if (fmt == "vtk_binary")
        fmt = "vtk";
    const bool loaded = (fmt == "auto") ? parseSnapshotWithFallback(inputPath, outParticles)
                                        : parseSnapshotByFormat(fmt, inputPath, outParticles);
    if (!loaded || outParticles.size() < 2) {
        std::cerr << "[server] failed to parse input state: " << inputPath
                  << " (format=" << (format.empty() ? "auto" : format) << ")\n";
        outParticles.clear();
        return false;
    }
    return true;
}

/*
 * @brief Documents the capture checkpoint to file operation contract.
 * @param outputPath Input value used by this contract.
 * @param outError Input value used by this contract.
 * @return bool SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationServer::captureCheckpointToFile(const std::string& outputPath, std::string* outError)
{
    if (!_system) {
        if (outError != nullptr) {
            *outError = "runtime is not ready";
        }
        return false;
    }
    if (!_system->syncHostState()) {
        if (outError != nullptr) {
            *outError = "could not synchronize host state";
        }
        return false;
    }
    SimulationCheckpointState state{};
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        state.config = _configState._runtimeConfigMirror;
        state.config.particleCount = _configState._particleCount;
        state.config.dt = _dt.load(std::memory_order_relaxed);
        state.config.substepTargetDt = _configuredSubstepTargetDt.load(std::memory_order_relaxed);
        state.config.maxSubsteps = _configuredMaxSubsteps.load(std::memory_order_relaxed);
        state.config.snapshotPublishPeriodMs =
            _snapshotPublishPeriodMs.load(std::memory_order_relaxed);
        state.config.solver = _configState._solverMode;
        state.config.integrator = _configState._integratorMode;
        state.config.performanceProfile = _configState._performanceProfile;
        state.config.octreeTheta = _configState._octreeTheta;
        state.config.octreeSoftening = _configState._octreeSoftening;
        state.config.octreeOpeningCriterion = _configState._octreeOpeningCriterion;
        state.config.octreeThetaAutoTune = _configState._octreeThetaAutoTune;
        state.config.octreeThetaAutoMin = _configState._octreeThetaAutoMin;
        state.config.octreeThetaAutoMax = _configState._octreeThetaAutoMax;
        state.config.sphEnabled = _configState._sphEnabled;
        state.config.sphSmoothingLength = _configState._sphSmoothingLength;
        state.config.sphRestDensity = _configState._sphRestDensity;
        state.config.sphGasConstant = _configState._sphGasConstant;
        state.config.sphViscosity = _configState._sphViscosity;
        state.config.energyMeasureEverySteps =
            _energyMeasureEverySteps.load(std::memory_order_relaxed);
        state.config.energySampleLimit = _energySampleLimit.load(std::memory_order_relaxed);
        state.gpuTelemetryEnabled = _gpuTelemetryEnabled.load(std::memory_order_relaxed);
        state.config.physicsMaxAcceleration = _configState._physicsMaxAcceleration;
        state.config.physicsMinSoftening = _configState._physicsMinSoftening;
        state.config.physicsMinDistance2 = _configState._physicsMinDistance2;
        state.config.physicsMinTheta = _configState._physicsMinTheta;
        state.config.sphMaxAcceleration = _configState._sphMaxAcceleration;
        state.config.sphMaxSpeed = _configState._sphMaxSpeed;
    }
    state.particles = _system->getParticles();
    state.steps = _steps.load(std::memory_order_relaxed);
    state.totalTime = _totalTime.load(std::memory_order_relaxed);
    state.paused = _paused.load(std::memory_order_relaxed);
    state.hasEnergyBaseline = _configState._hasEnergyBaseline;
    state.energyBaseline = _configState._energyBaseline;
    return writeCheckpointFile(outputPath, state, outError);
}
