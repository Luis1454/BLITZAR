/*
 * @file engine/batch/src/Runner.cpp
 * @brief Direct, synchronous execution path for the headless solver.
 */

#include "Runner.hpp"
#include "simulation/Internal.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bltzr_batch {
bool loadInitialParticles(const ResolvedInitialStatePlan& initialState,
                          std::vector<Particle>& particles)
{
    if (initialState.config.mode != "file" || initialState.inputFile.empty()) {
        return false;
    }
    if (!std::filesystem::is_regular_file(initialState.inputFile)) {
        std::cerr << "[batch] input file not found: " << initialState.inputFile << "\n";
        return false;
    }
    std::string format = normalizeSnapshotFormat(initialState.inputFormat);
    if (format == "auto") {
        format = normalizeSnapshotFormat(guessFormatFromPath(initialState.inputFile));
    }
    if (format == "vtk_binary") {
        format = "vtk";
    }
    const bool loaded = format == "auto"
                            ? parseSnapshotWithFallback(initialState.inputFile, particles)
                            : parseSnapshotByFormat(format, initialState.inputFile, particles);
    if (!loaded || particles.size() < 2u) {
        particles.clear();
        std::cerr << "[batch] failed to parse input state: " << initialState.inputFile << "\n";
        return false;
    }
    return true;
}

void configureSystem(ParticleSystem& system, const SimulationConfig& config,
                     const InitialStateConfig& initialState, std::string& solver,
                     std::string& integrator)
{
    solver = executableSolverMode(config.solver);
    integrator = config.integrator;
    coerceConfigSolverIntegratorCompatibility(solver, integrator, "batch");
    if (config.treePmEnabled && config.treePmModel == "exact_tree") {
        std::cerr << "[treepm] requested model=exact_tree; backend=" << solver
                  << " precision=" << config.treePmPrecision << "\n";
    }
    const std::vector<Particle>& particles = system.getParticles();
    const float theta = resolveOctreeTheta(config.octreeTheta, config.octreeThetaAutoTune,
                                           config.octreeThetaAutoMin, config.octreeThetaAutoMax,
                                           config.performanceProfile, particles,
                                           computeOctreeDistributionScore(particles));
    system.setLinearOctreeLeafCapacity(static_cast<int>(config.linearOctreeLeafCapacity));
    system.setTreePmParameters(
        config.treePmEnabled, config.treePmModel, config.treePmLayout, config.treePmPrecision,
        config.treePmAssignment,
        config.treePmLocalGrid,
        static_cast<int>(config.treePmGridSize),
        static_cast<int>(config.treePmJacobiIterations), config.treePmCutoffFactor,
        static_cast<int>(config.treePmMaxLocalNeighbors), static_cast<int>(config.treePmParticleLimit),
        static_cast<int>(config.treePmDenseCellThreshold), config.treePmGravityOnlyBuffers);
    system.setCudaCachePreference(config.cudaCachePreference);
    system.setSolverMode(solverModeFromCanonicalName(solver));
    system.setIntegratorMode(integratorModeFromCanonicalName(integrator));
    system.setOctreeTheta(theta);
    system.setOctreeSoftening(config.octreeSoftening);
    system.setOctreeOpeningCriterion(
        openingCriterionFromCanonicalName(config.octreeOpeningCriterion));
    system.setSphEnabled(config.sphEnabled);
    system.setDeterministicMode(config.deterministicMode);
    system.setSphParameters(config.sphSmoothingLength, config.sphRestDensity, config.sphGasConstant,
                            config.sphViscosity);
    system.setPhysicsStabilityConstants(config.physicsMaxAcceleration, config.physicsMinSoftening,
                                        config.physicsMinDistance2, config.physicsMinTheta);
    system.setSphCaps(config.sphMaxAcceleration, config.sphMaxSpeed);
    system.setAdaptiveTimeStepParameters(config.adaptiveTimeStepsEnabled,
                                         config.adaptiveTimeStepMaxLevel,
                                         config.adaptiveTimeStepEta);
    system.setAdaptiveTimeStepCostGuard(config.adaptiveTimeStepCostGuard);
    system.setThermalParameters(config.thermalAmbientTemperature, config.thermalSpecificHeat,
                                config.thermalHeatingCoeff, config.thermalRadiationCoeff);
    system.setCosmologyParameters(initialState.cosmology);
    if (!system.reconfigureRuntimeBuffers()) {
        throw std::runtime_error("[batch] failed to allocate configured runtime buffers");
    }
}

bool writeFinalState(ParticleSystem& system, const RunRequest& request, RunResult& result)
{
    if (!request.exportOnExit) {
        return true;
    }
    if (!system.syncHostState()) {
        result.error = "could not synchronize final particle state";
        return false;
    }
    AsyncExportJob job;
    job.outputPath = request.exportPath.empty()
                         ? defaultExportPath(request.config.exportDirectory,
                                             request.config.exportFormat, result.steps)
                         : request.exportPath;
    job.format = request.config.exportFormat;
    job.particles = system.getParticles();
    job.solverModeLabel = result.solver;
    job.integratorModeLabel = result.integrator;
    job.step = result.steps;
    if (!writeExportSnapshotFile(job)) {
        result.error = "could not write final state";
        return false;
    }
    result.exportPath = job.outputPath;
    return true;
}

RunResult Runner::run(const RunRequest& request) const
{
    RunResult result;
    const auto initializationStart = std::chrono::steady_clock::now();
    std::vector<Particle> particles;
    const bool loaded = loadInitialParticles(request.initialState, particles);
    if (!loaded) {
        buildGeneratedState(particles, std::max<std::uint32_t>(2u, request.config.particleCount),
                            request.initialState.config);
    }
    if (!particles.empty()) {
        const std::size_t beforeTransform = particles.size();
        if (applyInitialStateTransform(particles, request.initialState.config)) {
            std::cout << "[batch] scene transform particles=" << beforeTransform << " -> "
                      << particles.size() << " copies="
                      << request.initialState.config.sceneRotationCopies << " mirror="
                      << (request.initialState.config.sceneMirrorX ? "x" : "")
                      << (request.initialState.config.sceneMirrorY ? "y" : "")
                      << (request.initialState.config.sceneMirrorZ ? "z" : "") << " offset="
                      << request.initialState.config.sceneOffsetX << ','
                      << request.initialState.config.sceneOffsetY << ','
                      << request.initialState.config.sceneOffsetZ << "\n";
        }
    }
    for (Particle& particle : particles) {
        particle.setPressure(Vector3(0.0f, 0.0f, 0.0f));
        particle.setDensity(0.0f);
        particle.setTemperature(std::max(0.0f, particle.getTemperature()));
    }
    const std::string requestedSolver = executableSolverMode(request.config.solver);
    const bool solverUsesCudaRuntime =
        requestedSolver == "pairwise_cuda" || requestedSolver == "octree_gpu";
    const bool enableCudaRuntime = solverUsesCudaRuntime || request.config.sphEnabled;
    std::unique_ptr<ParticleSystem> system;
    if (particles.empty()) {
        particles.resize(std::max<std::uint32_t>(2u, request.config.particleCount));
        system = std::make_unique<ParticleSystem>(std::move(particles), enableCudaRuntime);
    }
    else {
        system = std::make_unique<ParticleSystem>(std::move(particles), enableCudaRuntime);
    }
    configureSystem(*system, request.config, request.initialState.config, result.solver,
                    result.integrator);
    result.executionBackend = enableCudaRuntime ? "cuda" : "cpu";
    result.particleCount = static_cast<std::uint32_t>(system->getParticles().size());
    const bool eulerIntegrator =
        system->getIntegratorMode() == ParticleSystem::IntegratorMode::Euler;
    const float targetDt =
        request.config.substepTargetDt > 0.0f
            ? request.config.substepTargetDt
            : autoTargetSubstepDt(result.solver, eulerIntegrator, request.config.sphEnabled,
                                  result.particleCount);
    const std::uint32_t substeps = std::min<std::uint32_t>(
        std::max<std::uint32_t>(
            1u, static_cast<std::uint32_t>(std::ceil(request.config.dt / targetDt))),
        std::max<std::uint32_t>(1u, request.config.maxSubsteps));
    const float substepDt = request.config.dt / static_cast<float>(substeps);
    result.initializationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                            std::chrono::steady_clock::now() - initializationStart)
                                            .count();
    const auto start = std::chrono::steady_clock::now();
    for (std::uint32_t step = 0u; step < request.targetSteps; ++step) {
        for (std::uint32_t substep = 0u; substep < substeps; ++substep) {
            if (!system->update(substepDt)) {
                result.faulted = true;
                result.error = "particle system update failed";
                break;
            }
            result.simulatedTime += substepDt;
        }
        if (result.faulted) {
            break;
        }
        ++result.steps;
    }
    result.integrationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::steady_clock::now() - start)
                                         .count();
    result.cosmologyScaleFactor = system->getCosmologyScaleFactor();
    if (!result.faulted) {
        const auto exportStart = std::chrono::steady_clock::now();
        writeFinalState(*system, request, result);
        result.exportMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::steady_clock::now() - exportStart)
                                        .count();
    }
    result.elapsedMilliseconds = result.initializationMilliseconds +
                                 result.integrationMilliseconds + result.exportMilliseconds;
    return result;
}
} // namespace bltzr_batch
