/*
 * @file engine/src/config/SimulationConfigDirectiveWrite.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/directive/Write.hpp"
#include "config/core/Config.hpp"
#include "config/directive/StreamWriter.hpp"
#include "config/profile/Performance.hpp"
#include "protocol/Protocol.hpp"
#include <algorithm>

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

static void writeSimulation(std::ostream& out, const SimulationConfig& config)
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

static void writePerformance(std::ostream& out, const SimulationConfig& config)
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

static void writeAdaptive(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "adaptive");
    writer.writeBool("enabled", config.adaptiveTimeStepsEnabled);
    writer.writeUint32("max_level", config.adaptiveTimeStepMaxLevel);
    writer.writeFloat("eta", config.adaptiveTimeStepEta);
    writer.writeBool("cost_guard", config.adaptiveTimeStepCostGuard);
    writer.finish();
}

static void writeOctree(std::ostream& out, const SimulationConfig& config)
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

static void writeTreePm(std::ostream& out, const SimulationConfig& config)
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

static void writePhysics(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "physics");
    writer.writeFloat("max_acceleration", config.physicsMaxAcceleration);
    writer.writeFloat("min_softening", config.physicsMinSoftening);
    writer.writeFloat("min_distance2", config.physicsMinDistance2);
    writer.writeFloat("min_theta", config.physicsMinTheta);
    writer.finish();
}

static void writeClient(std::ostream& out, const SimulationConfig& config)
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

static void writeExport(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "export");
    writer.writeQuotedString("directory", config.exportDirectory);
    writer.writeString("format", config.exportFormat);
    writer.finish();
}

static void writeScene(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "scene");
    writer.writeString("style", config.initConfigStyle);
    writer.writeString("preset", config.presetStructure);
    writer.writeString("mode", config.initMode);
    writer.writeQuotedString("file", config.inputFile);
    writer.writeString("format", config.inputFormat);
    writer.finish();
}

static void writePreset(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "preset");
    writer.writeFloat("size", config.presetSize);
    writer.writeFloat("velocity_temperature", config.velocityTemperature);
    writer.writeFloat("temperature", config.particleTemperature);
    writer.finish();
}

static void writeThermal(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "thermal");
    writer.writeFloat("ambient", config.thermalAmbientTemperature);
    writer.writeFloat("specific_heat", config.thermalSpecificHeat);
    writer.writeFloat("heating", config.thermalHeatingCoeff);
    writer.writeFloat("radiation", config.thermalRadiationCoeff);
    writer.finish();
}

static void writeGeneration(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "generation");
    writer.writeUint32("seed", config.initSeed);
    writer.writeBool("include_central_body", config.initIncludeCentralBody);
    writer.writeBool("deterministic", config.deterministicMode);
    writer.finish();
}

static void writeCentralBody(std::ostream& out, const SimulationConfig& config)
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

static void writeDisk(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "disk");
    writer.writeFloat("mass", config.initDiskMass);
    writer.writeFloat("radius_min", config.initDiskRadiusMin);
    writer.writeFloat("radius_max", config.initDiskRadiusMax);
    writer.writeFloat("thickness", config.initDiskThickness);
    writer.writeFloat("velocity_scale", config.initVelocityScale);
    writer.finish();
}

static void writeCloud(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "cloud");
    writer.writeFloat("half_extent", config.initCloudHalfExtent);
    writer.writeFloat("cube_half_extent", config.initCubeHalfExtent);
    writer.writeFloat("sphere_radius", config.initSphereRadius);
    writer.writeFloat("speed", config.initCloudSpeed);
    writer.writeFloat("particle_mass", config.initParticleMass);
    writer.finish();
}

static void writeCosmology(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "cosmology");
    writer.writeBool("enabled", config.cosmologyEnabled);
    writer.writeString("mode", config.cosmologyMode);
    writer.writeString("geometry", config.cosmologyGeometry);
    writer.writeFloat("box_half_extent", config.cosmologyBoxHalfExtent);
    writer.writeFloat("sphere_radius", config.cosmologySphereRadius);
    writer.writeFloat("h0", config.cosmologyHubbleH0);
    writer.writeFloat("omega_m", config.cosmologyOmegaMatter);
    writer.writeFloat("omega_lambda", config.cosmologyOmegaLambda);
    writer.writeFloat("omega_radiation", config.cosmologyOmegaRadiation);
    writer.writeFloat("initial_scale_factor", config.cosmologyInitialScaleFactor);
    writer.writeFloat("perturbation", config.cosmologyPerturbationAmplitude);
    writer.writeFloat("peculiar_velocity", config.cosmologyPeculiarVelocityScale);
    writer.writeString("mass_model", config.cosmologyMassModel);
    writer.writeFloat("total_mass", config.cosmologyTotalMass);
    writer.finish();
}

static void writeSceneObjects(std::ostream& out, const SimulationConfig& config)
{
    for (const SceneObjectConfig& object : config.scene.objects) {
        DirectiveStreamWriter writer(out, "object");
        writer.writeQuotedString("id", object.id);
        writer.writeQuotedString("name", object.name);
        writer.writeString("type", object.type);
        writer.writeBool("enabled", object.enabled);
        writer.writeBool("include_central_body", object.includeCentralBody);
        writer.writeUint32("particle_count", object.particleCount);
        writer.writeUint32("seed", object.seed);
        writer.writeFloat("mass", object.mass);
        writer.writeFloat("size", object.size);
        writer.writeFloat("radius_min", object.radiusMin);
        writer.writeFloat("radius_max", object.radiusMax);
        writer.writeFloat("thickness", object.thickness);
        writer.writeFloat("velocity_scale", object.velocityScale);
        writer.writeFloat("speed", object.speed);
        writer.writeFloat("particle_mass", object.particleMass);
        writer.writeFloat("x", object.positionX);
        writer.writeFloat("y", object.positionY);
        writer.writeFloat("z", object.positionZ);
        writer.writeFloat("vx", object.velocityX);
        writer.writeFloat("vy", object.velocityY);
        writer.writeFloat("vz", object.velocityZ);
        writer.writeBool("asset", object.isAsset);
        std::string properties;
        for (std::size_t index = 0u; index < object.properties.size(); ++index) {
            if (index != 0u)
                properties += ',';
            properties += object.properties[index];
        }
        writer.writeQuotedString("properties", properties);
        writer.writeFloat("offset_x", object.offsetX);
        writer.writeFloat("offset_y", object.offsetY);
        writer.writeFloat("offset_z", object.offsetZ);
        writer.writeFloat("rotation_x", object.rotationX);
        writer.writeFloat("rotation_y", object.rotationY);
        writer.writeFloat("rotation_z", object.rotationZ);
        writer.writeString("copy_axis", object.axis);
        writer.writeUint32("rotation_copies", object.copies);
        writer.writeBool("mirror_x", object.mirrorX);
        writer.writeBool("mirror_y", object.mirrorY);
        writer.writeBool("mirror_z", object.mirrorZ);
        writer.writeString("pivot", object.pivot);
        writer.writeFloat("pivot_x", object.pivotX);
        writer.writeFloat("pivot_y", object.pivotY);
        writer.writeFloat("pivot_z", object.pivotZ);
        if (!object.assetId.empty())
            writer.writeQuotedString("asset_id", object.assetId);
        if (object.type == "particle_system") {
            writer.writeString("distribution", object.distribution);
            writer.writeFloat("particle_size", object.particleSize);
            writer.writeFloat("particle_height", object.particleHeight);
            writer.writeFloat("particle_speed", object.particleSpeed);
            writer.writeQuotedString("emitter_object_id", object.emitterObjectId);
            writer.writeQuotedString("target_asset_id", object.targetAssetId);
        }
        writer.finish();
    }
}

static void writeTransform(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "transform");
    writer.writeFloat("offset_x", config.sceneOffsetX);
    writer.writeFloat("offset_y", config.sceneOffsetY);
    writer.writeFloat("offset_z", config.sceneOffsetZ);
    writer.writeFloat("rotation_x", config.sceneRotationX);
    writer.writeFloat("rotation_y", config.sceneRotationY);
    writer.writeFloat("rotation_z", config.sceneRotationZ);
    writer.writeString("copy_axis", config.sceneCopyAxis);
    writer.writeUint32("rotation_copies", config.sceneRotationCopies);
    writer.writeBool("mirror_x", config.sceneMirrorX);
    writer.writeBool("mirror_y", config.sceneMirrorY);
    writer.writeBool("mirror_z", config.sceneMirrorZ);
    writer.finish();
}

static void writeSph(std::ostream& out, const SimulationConfig& config)
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

static void writeRender(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "render");
    writer.writeBool("culling", config.renderCullingEnabled);
    writer.writeBool("lod", config.renderLODEnabled);
    writer.writeFloat("lod_near", config.renderLODNearDistance);
    writer.writeFloat("lod_far", config.renderLODFarDistance);
    writer.finish();
}

void SimulationConfigDirective::write(std::ostream& out, const SimulationConfig& config)
{
    out << "# ==================================================\n";
    out << "# BLITZAR directive config\n";
    out << "# Generated automatically. Edit values then restart.\n";
    out << "# ==================================================\n\n";
    writeSimulation(out, config);
    writePerformance(out, config);
    writeAdaptive(out, config);
    writeOctree(out, config);
    writeTreePm(out, config);
    writePhysics(out, config);
    writeClient(out, config);
    writeExport(out, config);
    writeScene(out, config);
    writeSceneObjects(out, config);
    writePreset(out, config);
    writeThermal(out, config);
    writeGeneration(out, config);
    writeCentralBody(out, config);
    writeDisk(out, config);
    writeCloud(out, config);
    writeCosmology(out, config);
    writeTransform(out, config);
    writeSph(out, config);
    writeRender(out, config);
}
} // namespace bltzr_config
