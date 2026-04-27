// File: engine/src/server/simulation_server/LoadAndCheckpoint.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "Internal.hpp"

/// Description: Executes the getRuntimeConfig operation.
SimulationConfig SimulationServer::getRuntimeConfig() const
{
    SimulationConfig config;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        config = _runtimeConfigMirror;
        config.particleCount = _particleCount;
        config.solver = _solverMode;
        config.integrator = _integratorMode;
        config.performanceProfile = _performanceProfile;
        config.octreeTheta = _octreeTheta;
        config.octreeSoftening = _octreeSoftening;
        config.octreeOpeningCriterion = _octreeOpeningCriterion;
        config.octreeThetaAutoTune = _octreeThetaAutoTune;
        config.octreeThetaAutoMin = _octreeThetaAutoMin;
        config.octreeThetaAutoMax = _octreeThetaAutoMax;
        config.sphEnabled = _sphEnabled;
        config.sphSmoothingLength = _sphSmoothingLength;
        config.sphRestDensity = _sphRestDensity;
        config.sphGasConstant = _sphGasConstant;
        config.sphViscosity = _sphViscosity;
        config.exportDirectory = _exportDirectory;
        config.exportFormat = _exportFormatDefault;
        config.inputFile = _initialStatePath;
        config.inputFormat = _initialStateFormat;
    }
    config.dt = _dt.load(std::memory_order_relaxed);
    config.substepTargetDt = _configuredSubstepTargetDt.load(std::memory_order_relaxed);
    config.maxSubsteps = _configuredMaxSubsteps.load(std::memory_order_relaxed);
    config.snapshotPublishPeriodMs = _snapshotPublishPeriodMs.load(std::memory_order_relaxed);
    config.energyMeasureEverySteps = _energyMeasureEverySteps.load(std::memory_order_relaxed);
    config.energySampleLimit = _energySampleLimit.load(std::memory_order_relaxed);
    config.physicsMaxAcceleration = _physicsMaxAcceleration;
    config.physicsMinSoftening = _physicsMinSoftening;
    config.physicsMinDistance2 = _physicsMinDistance2;
    config.physicsMinTheta = _physicsMinTheta;
    config.sphMaxAcceleration = _sphMaxAcceleration;
    config.sphMaxSpeed = _sphMaxSpeed;
    return config;
}

/// Description: Describes the load initial state operation contract.
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

/// Description: Executes the captureCheckpointToFile operation.
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
        state.config = _runtimeConfigMirror;
        state.config.particleCount = _particleCount;
        state.config.dt = _dt.load(std::memory_order_relaxed);
        state.config.substepTargetDt = _configuredSubstepTargetDt.load(std::memory_order_relaxed);
        state.config.maxSubsteps = _configuredMaxSubsteps.load(std::memory_order_relaxed);
        state.config.snapshotPublishPeriodMs =
            _snapshotPublishPeriodMs.load(std::memory_order_relaxed);
        state.config.solver = _solverMode;
        state.config.integrator = _integratorMode;
        state.config.performanceProfile = _performanceProfile;
        state.config.octreeTheta = _octreeTheta;
        state.config.octreeSoftening = _octreeSoftening;
        state.config.octreeOpeningCriterion = _octreeOpeningCriterion;
        state.config.octreeThetaAutoTune = _octreeThetaAutoTune;
        state.config.octreeThetaAutoMin = _octreeThetaAutoMin;
        state.config.octreeThetaAutoMax = _octreeThetaAutoMax;
        state.config.sphEnabled = _sphEnabled;
        state.config.sphSmoothingLength = _sphSmoothingLength;
        state.config.sphRestDensity = _sphRestDensity;
        state.config.sphGasConstant = _sphGasConstant;
        state.config.sphViscosity = _sphViscosity;
        state.config.energyMeasureEverySteps =
            _energyMeasureEverySteps.load(std::memory_order_relaxed);
        state.config.energySampleLimit = _energySampleLimit.load(std::memory_order_relaxed);
        state.gpuTelemetryEnabled = _gpuTelemetryEnabled.load(std::memory_order_relaxed);
        state.config.physicsMaxAcceleration = _physicsMaxAcceleration;
        state.config.physicsMinSoftening = _physicsMinSoftening;
        state.config.physicsMinDistance2 = _physicsMinDistance2;
        state.config.physicsMinTheta = _physicsMinTheta;
        state.config.sphMaxAcceleration = _sphMaxAcceleration;
        state.config.sphMaxSpeed = _sphMaxSpeed;
    }
    state.particles = _system->getParticles();
    state.steps = _steps.load(std::memory_order_relaxed);
    state.totalTime = _totalTime.load(std::memory_order_relaxed);
    state.paused = _paused.load(std::memory_order_relaxed);
    state.hasEnergyBaseline = _hasEnergyBaseline;
    state.energyBaseline = _energyBaseline;
    return writeCheckpointFile(outputPath, state, outError);
}
