// File: runtime/src/ffi/BlitzarCore.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "config/SimulationModes.hpp"
#include "runtime/src/ffi/BlitzarCoreInternal.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <thread>
#include <vector>
static constexpr std::uint32_t kDefaultTimeoutMs = 3000u;

/// Description: Executes the normalizedConfig operation.
static blitzar_core_config_t normalizedConfig(const blitzar_core_config_t& config)
{
    const blitzar_core_config_t defaults = blitzar_core_default_config();
    blitzar_core_config_t normalized = config;
    normalized.particle_count =
        config.particle_count == 0u ? defaults.particle_count : config.particle_count;
    normalized.dt = config.dt > 0.0f ? config.dt : defaults.dt;
    normalized.solver_name = (config.solver_name != nullptr && config.solver_name[0] != '\0')
                                 ? config.solver_name
                                 : defaults.solver_name;
    normalized.integrator_name =
        (config.integrator_name != nullptr && config.integrator_name[0] != '\0')
            ? config.integrator_name
            : defaults.integrator_name;
    normalized.performance_profile =
        (config.performance_profile != nullptr && config.performance_profile[0] != '\0')
            ? config.performance_profile
            : defaults.performance_profile;
    normalized.substep_target_dt =
        config.substep_target_dt >= 0.0f ? config.substep_target_dt : defaults.substep_target_dt;
    normalized.max_substeps =
        config.max_substeps == 0u ? defaults.max_substeps : config.max_substeps;
    normalized.snapshot_publish_period_ms = config.snapshot_publish_period_ms == 0u
                                                ? defaults.snapshot_publish_period_ms
                                                : config.snapshot_publish_period_ms;
    return normalized;
}

/// Description: Executes the copyText operation.
static void copyText(const std::string& value, char* buffer, std::size_t capacity)
{
    if (buffer == nullptr || capacity == 0u) {
        return;
    }
    const std::size_t limit = std::min<std::size_t>(capacity - 1u, value.size());
    std::memcpy(buffer, value.data(), limit);
    buffer[limit] = '\0';
}

/// Description: Executes the fillStatus operation.
static void fillStatus(const SimulationStats& stats, blitzar_core_status_t& outStatus)
{
    outStatus.steps = stats.steps;
    outStatus.dt = stats.dt;
    outStatus.paused = stats.paused ? 1u : 0u;
    outStatus.faulted = stats.faulted ? 1u : 0u;
    outStatus.fault_step = stats.faultStep;
    outStatus.sph_enabled = stats.sphEnabled ? 1u : 0u;
    outStatus.server_fps = stats.serverFps;
    outStatus.substep_target_dt = stats.substepTargetDt;
    outStatus.substep_dt = stats.substepDt;
    outStatus.substeps = stats.substeps;
    outStatus.max_substeps = stats.maxSubsteps;
    outStatus.snapshot_publish_period_ms = stats.snapshotPublishPeriodMs;
    outStatus.particle_count = stats.particleCount;
    outStatus.kinetic_energy = stats.kineticEnergy;
    outStatus.potential_energy = stats.potentialEnergy;
    outStatus.thermal_energy = stats.thermalEnergy;
    outStatus.radiated_energy = stats.radiatedEnergy;
    outStatus.total_energy = stats.totalEnergy;
    outStatus.energy_drift_pct = stats.energyDriftPct;
    outStatus.energy_estimated = stats.energyEstimated ? 1u : 0u;
    copyText(stats.solverName, outStatus.solver_name, BLITZAR_CORE_TEXT_CAPACITY);
    copyText(stats.integratorName, outStatus.integrator_name, BLITZAR_CORE_TEXT_CAPACITY);
    copyText(stats.performanceProfile, outStatus.performance_profile, BLITZAR_CORE_TEXT_CAPACITY);
    copyText(stats.faultReason, outStatus.fault_reason, BLITZAR_CORE_ERROR_CAPACITY);
}

namespace grav_ffi {
/// Description: Executes the BlitzarCore operation.
BlitzarCore::BlitzarCore(const blitzar_core_config_t& config)
    : _server(std::max<std::uint32_t>(2u, normalizedConfig(config).particle_count),
              normalizedConfig(config).dt)
{
    const blitzar_core_config_t normalized = normalizedConfig(config);
    _server.setPaused(true);
    _server.start();
    if (waitForSnapshot(kDefaultTimeoutMs) != BLITZAR_CORE_OK) {
        _server.stop();
        return;
    }
    if (applyConfig(normalized) != BLITZAR_CORE_OK) {
        _server.stop();
    }
}

/// Description: Releases resources owned by BlitzarCore.
BlitzarCore::~BlitzarCore()
{
    _server.stop();
}

/// Description: Executes the applyConfig operation.
blitzar_core_result_t BlitzarCore::applyConfig(const blitzar_core_config_t& config)
{
    clearError();
    const blitzar_core_config_t normalized = normalizedConfig(config);
    std::string canonicalSolver;
    std::string canonicalIntegrator;
    if (!grav_modes::normalizeSolver(normalized.solver_name, canonicalSolver) ||
        !grav_modes::normalizeIntegrator(normalized.integrator_name, canonicalIntegrator) ||
        !grav_modes::isSupportedSolverIntegratorPair(canonicalSolver, canonicalIntegrator)) {
        setError("invalid solver/integrator pair");
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    _server.setParticleCount(normalized.particle_count);
    _server.setDt(normalized.dt);
    _server.setSolverMode(canonicalSolver);
    _server.setIntegratorMode(canonicalIntegrator);
    _server.setPerformanceProfile(normalized.performance_profile);
    _server.setSubstepPolicy(normalized.substep_target_dt, normalized.max_substeps);
    _server.setSnapshotPublishPeriodMs(normalized.snapshot_publish_period_ms);
    _server.setPaused(true);
    return waitForAppliedConfig(normalized, kDefaultTimeoutMs);
}

/// Description: Executes the getStatus operation.
blitzar_core_result_t BlitzarCore::getStatus(blitzar_core_status_t& outStatus) const
{
    clearError();
    fillStatus(_server.getStats(), outStatus);
    return BLITZAR_CORE_OK;
}

/// Description: Describes the get snapshot operation contract.
blitzar_core_result_t BlitzarCore::getSnapshot(std::size_t maxPoints,
                                               blitzar_core_snapshot_t& outSnapshot) const
{
    clearError();
    std::vector<RenderParticle> snapshot;
    if (!_server.copyLatestSnapshot(snapshot, maxPoints)) {
        setError("snapshot not ready");
        return BLITZAR_CORE_NOT_READY;
    }
    outSnapshot.count = snapshot.size();
    outSnapshot.particles = new blitzar_render_particle_t[outSnapshot.count];
    for (std::size_t index = 0; index < outSnapshot.count; ++index) {
        outSnapshot.particles[index] = blitzar_render_particle_t{snapshot[index].x,
                                                                 snapshot[index].y,
                                                                 snapshot[index].z,
                                                                 snapshot[index].mass,
                                                                 snapshot[index].pressureNorm,
                                                                 snapshot[index].temperature};
    }
    return BLITZAR_CORE_OK;
}
} // namespace grav_ffi
