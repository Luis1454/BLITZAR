#include "config/SimulationConfigDirectiveWrite.hpp"

#include "config/SimulationConfig.hpp"
#include "config/DirectiveStreamWriter.hpp"
#include "config/SimulationPerformanceProfile.hpp"
#include "protocol/ServerProtocol.hpp"

#include <algorithm>

namespace grav_config {

static bool matchesManagedPerformanceFields(const SimulationConfig &lhs, const SimulationConfig &rhs)
{
    return lhs.clientParticleCap == rhs.clientParticleCap
        && lhs.snapshotPublishPeriodMs == rhs.snapshotPublishPeriodMs
        && lhs.energyMeasureEverySteps == rhs.energyMeasureEverySteps
        && lhs.energySampleLimit == rhs.energySampleLimit
        && lhs.substepTargetDt == rhs.substepTargetDt
        && lhs.maxSubsteps == rhs.maxSubsteps;
}

static void writeSimulation(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "simulation");
    writer.writeUint32("particle_count", config.particleCount);
    writer.writeFloat("dt", config.dt);
    writer.writeString("solver", config.solver);
    writer.writeString("integrator", config.integrator);
    writer.finish();
}

static void writePerformance(std::ostream &out, const SimulationConfig &config)
{
    SimulationConfig profileReference = SimulationConfig::defaults();
    profileReference.performanceProfile = config.performanceProfile;
    applyPerformanceProfile(profileReference);
    const bool emitCustomProfile =
        config.performanceProfile == "custom" || !matchesManagedPerformanceFields(config, profileReference);
    const std::string effectiveProfile = emitCustomProfile ? "custom" : config.performanceProfile;

    out << "performance(profile=" << effectiveProfile;
    if (emitCustomProfile) {
        out << ", draw_cap=" << std::min<std::uint32_t>(grav_protocol::kSnapshotMaxPoints, std::max<std::uint32_t>(2u, config.clientParticleCap))
            << ", snapshot_ms=" << std::max<std::uint32_t>(1u, config.snapshotPublishPeriodMs)
            << ", energy_every=" << std::max<std::uint32_t>(1u, config.energyMeasureEverySteps)
            << ", sample_limit=" << std::max<std::uint32_t>(64u, config.energySampleLimit)
            << ", substep_target_dt=" << config.substepTargetDt
            << ", max_substeps=" << std::max<std::uint32_t>(1u, config.maxSubsteps);
    }
    out << ")\n";
}

static void writeOctree(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "octree");
    writer.writeFloat("theta", config.octreeTheta);
    writer.writeFloat("softening", config.octreeSoftening);
    writer.finish();
}

static void writePhysics(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "physics");
    writer.writeFloat("max_acceleration", config.physicsMaxAcceleration);
    writer.writeFloat("min_softening", config.physicsMinSoftening);
    writer.writeFloat("min_distance2", config.physicsMinDistance2);
    writer.writeFloat("min_theta", config.physicsMinTheta);
    writer.finish();
}

static void writeClient(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "client");
    writer.writeFloat("zoom", config.defaultZoom);
    writer.writeInt("luminosity", config.defaultLuminosity);
    writer.writeUint32("ui_fps", config.uiFpsLimit);
    writer.writeUint32("command_timeout_ms", config.clientRemoteCommandTimeoutMs);
    writer.writeUint32("status_timeout_ms", config.clientRemoteStatusTimeoutMs);
    writer.writeUint32("snapshot_timeout_ms", config.clientRemoteSnapshotTimeoutMs);
    writer.writeUint32("snapshot_queue", config.clientSnapshotQueueCapacity);
    writer.writeString("drop_policy", config.clientSnapshotDropPolicy);
    writer.finish();
}

static void writeExport(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "export");
    writer.writeQuotedString("directory", config.exportDirectory);
    writer.writeString("format", config.exportFormat);
    writer.finish();
}

static void writeScene(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "scene");
    writer.writeString("style", config.initConfigStyle);
    writer.writeString("preset", config.presetStructure);
    writer.writeString("mode", config.initMode);
    writer.writeQuotedString("file", config.inputFile);
    writer.writeString("format", config.inputFormat);
    writer.finish();
}

static void writePreset(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "preset");
    writer.writeFloat("size", config.presetSize);
    writer.writeFloat("velocity_temperature", config.velocityTemperature);
    writer.writeFloat("temperature", config.particleTemperature);
    writer.finish();
}

static void writeThermal(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "thermal");
    writer.writeFloat("ambient", config.thermalAmbientTemperature);
    writer.writeFloat("specific_heat", config.thermalSpecificHeat);
    writer.writeFloat("heating", config.thermalHeatingCoeff);
    writer.writeFloat("radiation", config.thermalRadiationCoeff);
    writer.finish();
}

static void writeGeneration(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "generation");
    writer.writeUint32("seed", config.initSeed);
    writer.writeBool("include_central_body", config.initIncludeCentralBody);
    writer.writeBool("deterministic", config.deterministicMode);
    writer.finish();
}

static void writeCentralBody(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "central_body");
    writer.writeFloat("mass", config.initCentralMass);
    writer.writeFloat("x", config.initCentralX);
    writer.writeFloat("y", config.initCentralY);
    writer.writeFloat("z", config.initCentralZ);
    writer.writeFloat("vx", config.initCentralVx);
    writer.writeFloat("vy", config.initCentralVy);
    writer.writeFloat("vz", config.initCentralVz);
    writer.finish();
}

static void writeDisk(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "disk");
    writer.writeFloat("mass", config.initDiskMass);
    writer.writeFloat("radius_min", config.initDiskRadiusMin);
    writer.writeFloat("radius_max", config.initDiskRadiusMax);
    writer.writeFloat("thickness", config.initDiskThickness);
    writer.writeFloat("velocity_scale", config.initVelocityScale);
    writer.finish();
}

static void writeCloud(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "cloud");
    writer.writeFloat("half_extent", config.initCloudHalfExtent);
    writer.writeFloat("speed", config.initCloudSpeed);
    writer.writeFloat("particle_mass", config.initParticleMass);
    writer.finish();
}

static void writeSph(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "sph");
    writer.writeBool("enabled", config.sphEnabled);
    writer.writeFloat("smoothing_length", config.sphSmoothingLength);
    writer.writeFloat("rest_density", config.sphRestDensity);
    writer.writeFloat("gas_constant", config.sphGasConstant);
    writer.writeFloat("viscosity", config.sphViscosity);
    writer.writeFloat("max_acceleration", config.sphMaxAcceleration);
    writer.writeFloat("max_speed", config.sphMaxSpeed);
    writer.finish();
}

static void writeRender(std::ostream &out, const SimulationConfig &config)
{
    DirectiveStreamWriter writer(out, "render");
    writer.writeBool("culling", config.renderCullingEnabled);
    writer.writeBool("lod", config.renderLODEnabled);
    writer.writeFloat("lod_near", config.renderLODNearDistance);
    writer.writeFloat("lod_far", config.renderLODFarDistance);
    writer.finish();
}

void SimulationConfigDirective::write(std::ostream &out, const SimulationConfig &config)
{
    out << "# ==================================================\n";
    out << "# BLITZAR directive config\n";
    out << "# Generated automatically. Edit values then restart.\n";
    out << "# ==================================================\n\n";
    writeSimulation(out, config);
    writePerformance(out, config);
    writeOctree(out, config);
    writePhysics(out, config);
    writeClient(out, config);
    writeExport(out, config);
    writeScene(out, config);
    writePreset(out, config);
    writeThermal(out, config);
    writeGeneration(out, config);
    writeCentralBody(out, config);
    writeDisk(out, config);
    writeCloud(out, config);
    writeSph(out, config);
    writeRender(out, config);
}

} // namespace grav_config
