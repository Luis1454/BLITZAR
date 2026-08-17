/*
 * @file engine/config/directive/CfgInitialState.cpp
 * @brief Serialization of initial-state, cosmology, fluid, and render directives.
 */

#include "CfgWriteInternals.hpp"

#include "config/core/CfgConfig.hpp"
#include "config/directive/CfgStreamWriter.hpp"

namespace bltzr_config {
void writePreset(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "preset");
    writer.writeFloat("size", config.presetSize);
    writer.writeFloat("velocity_temperature", config.velocityTemperature);
    writer.writeFloat("temperature", config.particleTemperature);
    writer.finish();
}

void writeThermal(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "thermal");
    writer.writeFloat("ambient", config.thermalAmbientTemperature);
    writer.writeFloat("specific_heat", config.thermalSpecificHeat);
    writer.writeFloat("heating", config.thermalHeatingCoeff);
    writer.writeFloat("radiation", config.thermalRadiationCoeff);
    writer.finish();
}

void writeGeneration(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "generation");
    writer.writeUint32("seed", config.initSeed);
    writer.writeBool("include_central_body", config.initIncludeCentralBody);
    writer.writeBool("deterministic", config.deterministicMode);
    writer.finish();
}

void writeCentralBody(std::ostream& out, const SimulationConfig& config)
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

void writeDisk(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "disk");
    writer.writeFloat("mass", config.initDiskMass);
    writer.writeFloat("radius_min", config.initDiskRadiusMin);
    writer.writeFloat("radius_max", config.initDiskRadiusMax);
    writer.writeFloat("thickness", config.initDiskThickness);
    writer.writeFloat("velocity_scale", config.initVelocityScale);
    writer.finish();
}

void writeCloud(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "cloud");
    writer.writeFloat("half_extent", config.initCloudHalfExtent);
    writer.writeFloat("cube_half_extent", config.initCubeHalfExtent);
    writer.writeFloat("sphere_radius", config.initSphereRadius);
    writer.writeFloat("speed", config.initCloudSpeed);
    writer.writeFloat("particle_mass", config.initParticleMass);
    writer.finish();
}

void writeCosmology(std::ostream& out, const SimulationConfig& config)
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

void writeSph(std::ostream& out, const SimulationConfig& config)
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

void writeRender(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "render");
    writer.writeBool("culling", config.renderCullingEnabled);
    writer.writeBool("lod", config.renderLODEnabled);
    writer.writeFloat("lod_near", config.renderLODNearDistance);
    writer.writeFloat("lod_far", config.renderLODFarDistance);
    writer.finish();
}
} // namespace bltzr_config
