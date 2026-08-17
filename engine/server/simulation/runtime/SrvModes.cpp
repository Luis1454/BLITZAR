/*
 * @file engine/server/simulation/runtime/SrvModes.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Simulation server runtime modes operations.
 */

#include "simulation/SrvInternal.hpp"

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
        std::string nextSolver = executableSolverMode(canonical);
        if (!bltzr_modes::isSupportedSolverIntegratorPair(nextSolver, _configState._integratorMode)) {
            std::cerr << "[server] rejected solver octree_gpu because integrator rk4 is not "
                         "supported with it\n";
            return;
        }
        if (_configState._solverMode != nextSolver) {
            _configState._solverMode = nextSolver;
            changed = true;
        }
        _configState._runtimeConfigMirror.solver = _configState._solverMode;
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
        if (!bltzr_modes::isSupportedSolverIntegratorPair(_configState._solverMode, nextIntegrator)) {
            std::cerr << "[server] rejected integrator rk4 because solver octree_gpu supports "
                         "euler only\n";
            return;
        }
        if (_configState._integratorMode != nextIntegrator) {
            _configState._integratorMode = nextIntegrator;
            changed = true;
        }
        _configState._runtimeConfigMirror.integrator = _configState._integratorMode;
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
    _configState._performanceProfile = canonical;
    _configState._runtimeConfigMirror.performanceProfile = canonical;
}

void SimulationServer::setTreePmAssignment(const std::string& assignment)
{
    std::string canonical;
    if (!bltzr_config::normalizeTreePmAssignment(assignment, canonical)) {
        std::cerr << "[server] ignored invalid TreePM assignment: " << assignment << "\n";
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        if (_configState._runtimeConfigMirror.treePmAssignment != canonical) {
            _configState._runtimeConfigMirror.treePmAssignment = canonical;
            changed = true;
        }
    }
    if (changed && _running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

void SimulationServer::setTreePmParameters(bool enabled, const std::string& model,
                                            const std::string& layout,
                                            const std::string& precision,
                                            const std::string& assignment, bool localGrid,
                                            std::uint32_t gridSize,
                                            std::uint32_t jacobiIterations, float cutoffFactor,
                                            std::uint32_t maxLocalNeighbors,
                                            std::uint32_t particleLimit,
                                            std::uint32_t denseCellThreshold,
                                            bool gravityOnlyBuffers)
{
    std::string canonicalModel;
    std::string canonicalLayout;
    std::string canonicalPrecision;
    std::string canonicalAssignment;
    if (!bltzr_config::normalizeTreePmModel(model, canonicalModel) ||
        !bltzr_config::normalizeTreePmLayout(layout, canonicalLayout) ||
        !bltzr_config::normalizeTreePmPrecision(precision, canonicalPrecision) ||
        !bltzr_config::normalizeTreePmAssignment(assignment, canonicalAssignment)) {
        std::cerr << "[server] ignored invalid TreePM parameter set\n";
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        SimulationConfig& config = _configState._runtimeConfigMirror;
        changed = config.treePmEnabled != enabled || config.treePmModel != canonicalModel ||
                  config.treePmLayout != canonicalLayout ||
                  config.treePmPrecision != canonicalPrecision ||
                  config.treePmAssignment != canonicalAssignment ||
                  config.treePmLocalGrid != localGrid || config.treePmGridSize != gridSize ||
                  config.treePmJacobiIterations != jacobiIterations ||
                  std::abs(config.treePmCutoffFactor - cutoffFactor) > 1e-6f ||
                  config.treePmMaxLocalNeighbors != maxLocalNeighbors ||
                  config.treePmParticleLimit != particleLimit ||
                  config.treePmDenseCellThreshold != denseCellThreshold ||
                  config.treePmGravityOnlyBuffers != gravityOnlyBuffers;
        config.treePmEnabled = enabled;
        config.treePmPreset = "custom";
        config.treePmModel = canonicalModel;
        config.treePmLayout = canonicalLayout;
        config.treePmPrecision = canonicalPrecision;
        config.treePmAssignment = canonicalAssignment;
        config.treePmLocalGrid = localGrid;
        config.treePmGridSize = std::clamp(gridSize, 16u, 256u);
        config.treePmJacobiIterations = std::min(jacobiIterations, 128u);
        config.treePmCutoffFactor = std::clamp(cutoffFactor, 0.0f, 8.0f);
        config.treePmMaxLocalNeighbors = std::min(maxLocalNeighbors, 256u);
        config.treePmParticleLimit = std::min(particleLimit, 100000000u);
        config.treePmDenseCellThreshold = std::clamp(denseCellThreshold, 1u, 4096u);
        config.treePmGravityOnlyBuffers = gravityOnlyBuffers;
    }
    if (changed && _running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

void SimulationServer::setAdaptiveTimeStepParameters(bool enabled, std::uint32_t maxLevel,
                                                     float eta)
{
    const std::uint32_t safeLevel = std::min<std::uint32_t>(maxLevel, 12u);
    const float safeEta = std::clamp(eta, 0.01f, 1.0f);
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        changed = _configState._adaptiveTimeStepsEnabled != enabled ||
                  _configState._adaptiveTimeStepMaxLevel != safeLevel ||
                  std::abs(_configState._adaptiveTimeStepEta - safeEta) > 1e-6f;
        _configState._adaptiveTimeStepsEnabled = enabled;
        _configState._adaptiveTimeStepMaxLevel = safeLevel;
        _configState._adaptiveTimeStepEta = safeEta;
        _configState._runtimeConfigMirror.adaptiveTimeStepsEnabled = enabled;
        _configState._runtimeConfigMirror.adaptiveTimeStepMaxLevel = safeLevel;
        _configState._runtimeConfigMirror.adaptiveTimeStepEta = safeEta;
    }
    if (changed && _running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}

void SimulationServer::setAdaptiveTimeStepCostGuard(bool enabled)
{
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        changed = _configState._runtimeConfigMirror.adaptiveTimeStepCostGuard != enabled;
        _configState._runtimeConfigMirror.adaptiveTimeStepCostGuard = enabled;
    }
    if (changed && _running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}
