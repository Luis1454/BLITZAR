/*
 * @file engine/config/validation/src/validation/ScenarioInitialState.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Responsibility-focused scenario validation rules.
 */

#include "ScenarioInternals.hpp"

namespace bltzr_config {

void validateInitialState(const ResolvedInitialStatePlan& plan, bool requestedFileMode,
                          ScenarioValidationContext& context)
{
    if (plan.config.velocityTemperature < 0.0f) {
        context.add(ScenarioDiagnosticLevel::Error, "velocity_temperature",
                    "velocity_temperature [m/s] cannot be negative.",
                    "Set velocity_temperature to 0 m/s or a positive velocity scale.");
    }
    if (plan.config.particleTemperature < 0.0f) {
        context.add(ScenarioDiagnosticLevel::Error, "particle_temperature",
                    "particle_temperature [K] cannot be negative.",
                    "Set particle_temperature to 0 K or a positive temperature.");
    }
    if (plan.config.thermalAmbientTemperature < 0.0f) {
        context.add(ScenarioDiagnosticLevel::Error, "thermal_ambient_temperature",
                    "thermal_ambient_temperature [K] cannot be negative.",
                    "Set thermal_ambient_temperature to 0 K or above.");
    }
    if (!(plan.config.thermalSpecificHeat > 0.0f)) {
        context.add(ScenarioDiagnosticLevel::Error, "thermal_specific_heat",
                    "thermal_specific_heat [J/(kg*K)] must be strictly positive.",
                    "Set thermal_specific_heat to a value above 0 J/(kg*K).");
    }
    if (plan.config.thermalHeatingCoeff < 0.0f) {
        context.add(ScenarioDiagnosticLevel::Error, "thermal_heating_coeff",
                    "thermal_heating_coeff must be non-negative.",
                    "Set thermal_heating_coeff to 0 or a positive value.");
    }
    if (plan.config.thermalRadiationCoeff < 0.0f) {
        context.add(ScenarioDiagnosticLevel::Error, "thermal_radiation_coeff",
                    "thermal_radiation_coeff must be non-negative.",
                    "Set thermal_radiation_coeff to 0 or a positive value.");
    }
    if (requestedFileMode && plan.config.mode != "file") {
        context.add(
            ScenarioDiagnosticLevel::Error, "input_file",
            "File-based initialization was requested but no usable input_file was provided.",
            "Set input_file to an existing snapshot or switch init mode away from file.");
    }
    if (plan.config.mode != "file" && plan.config.particleMass <= 0.0f) {
        context.add(
            ScenarioDiagnosticLevel::Error, "init_particle_mass",
            "Generated scenarios require a strictly positive particle mass.",
            "Raise init_particle_mass or choose a preset that computes masses automatically.");
    }
    if (plan.config.includeCentralBody && plan.config.centralMass <= 0.0f) {
        context.add(ScenarioDiagnosticLevel::Error, "init_central_mass",
                    "Central body mass must be strictly positive when the central body is enabled.",
                    "Raise init_central_mass or disable the central body.");
    }
    if (plan.config.mode == "disk_orbit") {
        if (plan.config.diskMass <= 0.0f) {
            context.add(ScenarioDiagnosticLevel::Error, "init_disk_mass",
                        "Disk orbit scenarios require a strictly positive disk mass.",
                        "Raise init_disk_mass before running the scenario.");
        }
        if (plan.config.diskRadiusMax <= plan.config.diskRadiusMin) {
            context.add(ScenarioDiagnosticLevel::Error, "init_disk_radius",
                        "Disk radius max must be greater than disk radius min.",
                        "Increase init_disk_radius_max or lower init_disk_radius_min.");
        }
    }
    if ((plan.config.mode == "random_cloud" || plan.config.mode == "plummer_sphere") &&
        plan.config.cloudHalfExtent <= 0.0f) {
        context.add(ScenarioDiagnosticLevel::Error, "init_cloud_half_extent",
                    "Cloud-based presets require a strictly positive spatial extent.",
                    "Set init_cloud_half_extent above 0.");
    }
    if (plan.config.mode == "cube_random" && plan.config.cubeHalfExtent <= 0.0f) {
        context.add(ScenarioDiagnosticLevel::Error, "init_cube_half_extent",
                    "Cube random mode requires a strictly positive half extent.",
                    "Set init_cube_half_extent above 0.");
    }
    if (plan.config.mode == "sphere_random" && plan.config.sphereRadius <= 0.0f) {
        context.add(ScenarioDiagnosticLevel::Error, "init_sphere_radius",
                    "Sphere random mode requires a strictly positive radius.",
                    "Set init_sphere_radius above 0.");
    }
}

} // namespace bltzr_config
