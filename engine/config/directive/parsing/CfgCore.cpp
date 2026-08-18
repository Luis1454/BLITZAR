/*
 * @file engine/config/directive/parsing/CfgCore.cpp
 * @brief Serialization of runtime and solver configuration directives.
 */

#include "config/directive/write/CfgWriteInternals.hpp"

#include "config/core/configuration/CfgConfig.hpp"
#include "config/directive/write/CfgStreamWriter.hpp"
#include "config/profile/profile/CfgPerformance.hpp"
#include "protocol/PtcProtocol.hpp"

#include <algorithm>
#include <ostream>

namespace bltzr_config {
static bool matchesManagedPerformanceFields(const SimulationConfig& lhs,
                                            const SimulationConfig& rhs)
{
    return lhs.clientParticleCap == rhs.clientParticleCap &&
           lhs.snapshotPublishPeriodMs == rhs.snapshotPublishPeriodMs &&
           lhs.energyMeasureEverySteps == rhs.energyMeasureEverySteps &&
           lhs.energySampleLimit == rhs.energySampleLimit &&
           lhs.substepTargetDt == rhs.substepTargetDt && lhs.maxSubsteps == rhs.maxSubsteps;
}

void writeSimulation(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "simulation");
    writer.writeUint32("particle_count", config.particleCount);
    writer.writeFloat("dt", config.dt);
    writer.writeString("solver", config.solver);
    writer.writeString("integrator", config.integrator);
    if (!config.simulationProfile.empty()) {
        writer.writeString("profile", config.simulationProfile);
    }
    writer.finish();
}

void writePerformance(std::ostream& out, const SimulationConfig& config)
{
    SimulationConfig profileReference = SimulationConfig::defaults();
    profileReference.performanceProfile = config.performanceProfile;
    applyPerformanceProfile(profileReference);
    const bool emitCustomProfile = config.performanceProfile == "custom" ||
                                   !matchesManagedPerformanceFields(config, profileReference);
    const std::string effectiveProfile = emitCustomProfile ? "custom" : config.performanceProfile;
    out << "performance(profile=" << effectiveProfile;
    if (emitCustomProfile) {
        out << ", draw_cap="
            << std::min<std::uint32_t>(bltzr_protocol::kSnapshotMaxPoints,
                                       std::max<std::uint32_t>(2u, config.clientParticleCap))
            << ", snapshot_ms=" << std::max<std::uint32_t>(1u, config.snapshotPublishPeriodMs)
            << ", energy_every=" << std::max<std::uint32_t>(1u, config.energyMeasureEverySteps)
            << ", sample_limit=" << std::max<std::uint32_t>(64u, config.energySampleLimit)
            << ", substep_target_dt=" << config.substepTargetDt
            << ", max_substeps=" << std::max<std::uint32_t>(1u, config.maxSubsteps);
    }
    out << ")\n";
}

void writeAdaptive(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "adaptive");
    writer.writeBool("enabled", config.adaptiveTimeStepsEnabled);
    writer.writeUint32("max_level", config.adaptiveTimeStepMaxLevel);
    writer.writeFloat("eta", config.adaptiveTimeStepEta);
    writer.writeBool("cost_guard", config.adaptiveTimeStepCostGuard);
    writer.finish();
}

void writeOctree(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "octree");
    writer.writeFloat("theta", config.octreeTheta);
    writer.writeFloat("softening", config.octreeSoftening);
    writer.writeString("criterion", config.octreeOpeningCriterion);
    writer.writeBool("theta_auto", config.octreeThetaAutoTune);
    writer.writeFloat("theta_auto_min", config.octreeThetaAutoMin);
    writer.writeFloat("theta_auto_max", config.octreeThetaAutoMax);
    writer.writeUint32("leaf_capacity", config.linearOctreeLeafCapacity);
    writer.writeString("cache_preference", config.cudaCachePreference);
    writer.finish();
}

void writeTreePm(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "treepm");
    writer.writeString("preset", config.treePmPreset);
    writer.writeBool("enabled", config.treePmEnabled);
    writer.writeString("model", config.treePmModel);
    writer.writeString("layout", config.treePmLayout);
    writer.writeString("precision", config.treePmPrecision);
    writer.writeString("assignment", config.treePmAssignment);
    writer.writeBool("local_grid", config.treePmLocalGrid);
    writer.writeUint32("grid_size", config.treePmGridSize);
    writer.writeUint32("jacobi_iters", config.treePmJacobiIterations);
    writer.writeFloat("cutoff_factor", config.treePmCutoffFactor);
    writer.writeUint32("max_local_neighbors", config.treePmMaxLocalNeighbors);
    writer.writeUint32("particle_limit", config.treePmParticleLimit);
    writer.writeUint32("dense_cell_threshold", config.treePmDenseCellThreshold);
    writer.writeBool("gravity_only_buffers", config.treePmGravityOnlyBuffers);
    writer.finish();
}

void writePhysics(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "physics");
    writer.writeFloat("max_acceleration", config.physicsMaxAcceleration);
    writer.writeFloat("min_softening", config.physicsMinSoftening);
    writer.writeFloat("min_distance2", config.physicsMinDistance2);
    writer.writeFloat("min_theta", config.physicsMinTheta);
    writer.finish();
}

void writeClient(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "client");
    writer.writeFloat("zoom", config.defaultZoom);
    writer.writeInt("luminosity", config.defaultLuminosity);
    writer.writeString("theme", config.uiTheme);
    writer.writeUint32("ui_fps", config.uiFpsLimit);
    writer.writeUint32("command_timeout_ms", config.clientRemoteCommandTimeoutMs);
    writer.writeUint32("status_timeout_ms", config.clientRemoteStatusTimeoutMs);
    writer.writeUint32("snapshot_timeout_ms", config.clientRemoteSnapshotTimeoutMs);
    writer.writeUint32("snapshot_queue", config.clientSnapshotQueueCapacity);
    writer.writeString("drop_policy", config.clientSnapshotDropPolicy);
    writer.finish();
}

void writeExport(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "export");
    writer.writeQuotedString("directory", config.exportDirectory);
    writer.writeString("format", config.exportFormat);
    writer.finish();
}
} // namespace bltzr_config
