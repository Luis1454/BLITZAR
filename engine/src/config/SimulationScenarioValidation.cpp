/*
 * @file engine/src/config/SimulationScenarioValidation.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/SimulationScenarioValidation.hpp"
#include "config/SimulationConfig.hpp"
#include "config/SimulationModes.hpp"
#include "config/SimulationScenarioValidationPhysics.hpp"
#include "config/SimulationScenarioValidationRender.hpp"
#include "server/SimulationInitConfig.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <utility>

namespace grav_config {
class SimulationScenarioValidationLocal final {
public:
    static ScenarioValidationReport evaluate(const SimulationConfig& config)
    {
        ScenarioValidationReport report;
        const auto addDiagnostic = [&report](ScenarioDiagnosticLevel level, std::string field,
                                             std::string message, std::string action) {
            ScenarioDiagnostic diagnostic;
            diagnostic.level = level;
            diagnostic.field = std::move(field);
            diagnostic.message = std::move(message);
            diagnostic.action = std::move(action);
            report.diagnostics.push_back(std::move(diagnostic));
            if (level == ScenarioDiagnosticLevel::Error) {
                report.errorCount += 1u;
                report.validForRun = false;
            }
            else {
                report.warningCount += 1u;
            }
        };
        std::ostringstream initLog;
        const ResolvedInitialStatePlan plan = resolveInitialStatePlan(config, initLog);
        std::istringstream logLines(initLog.str());
        for (std::string line; std::getline(logLines, line);) {
            if (!line.empty()) {
                addDiagnostic(ScenarioDiagnosticLevel::Warning, "initial_state", std::move(line),
                              "Review init mode, preset selection, and input file settings.");
            }
        }
        const auto normalized = [](std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        };
        const std::string initStyle = normalized(config.initConfigStyle);
        const std::string presetMode = normalized(config.presetStructure);
        const std::string detailedMode = normalized(config.initMode);
        const std::string snapshotDropPolicy = normalized(config.clientSnapshotDropPolicy);
        const bool requestedFileMode = (initStyle == "preset" && presetMode == "file") ||
                                       (initStyle != "preset" && detailedMode == "file");
        if (config.particleCount < 2u) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "particle_count",
                          "At least 2 particles are required to run the simulation.",
                          "Increase particle_count to 2 or more.");
        }
        else if (config.particleCount < 16u) {
            addDiagnostic(ScenarioDiagnosticLevel::Warning, "particle_count",
                          "Very low particle counts can make scenario behavior unrepresentative.",
                          "Use a larger particle_count for stable tuning and visualization.");
        }
        if (!(config.dt > 0.0f)) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "dt",
                          "Time step dt [s] must be strictly positive.",
                          "Set dt to a value above 0 s.");
        }
        else if (config.dt > 0.05f) {
            addDiagnostic(ScenarioDiagnosticLevel::Warning, "dt",
                          "Large time steps dt [s] increase the risk of unstable orbits and missed "
                          "interactions.",
                          "Reduce dt [s] or enable a tighter substep_target_dt [s].");
        }
        if (config.substepTargetDt < 0.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "substep_target_dt",
                          "Substep target dt cannot be negative.",
                          "Use 0 for auto mode or a positive target dt.");
        }
        else if (config.substepTargetDt > 0.0f && config.substepTargetDt > config.dt) {
            addDiagnostic(ScenarioDiagnosticLevel::Warning, "substep_target_dt",
                          "Configured substep target dt is larger than dt, so no finer subdivision "
                          "will happen.",
                          "Lower substep_target_dt or leave it at 0 for auto mode.");
        }
        if (config.maxSubsteps == 0u) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "max_substeps",
                          "Maximum substeps cannot be 0.", "Set max_substeps to at least 1.");
        }
        if (config.clientSnapshotQueueCapacity == 0u) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "client_snapshot_queue_capacity",
                          "Snapshot queue capacity must be at least 1 frame.",
                          "Set client_snapshot_queue_capacity to 1 or more.");
        }
        else if (config.clientSnapshotQueueCapacity > 16u) {
            addDiagnostic(ScenarioDiagnosticLevel::Warning, "client_snapshot_queue_capacity",
                          "Large snapshot queue capacities increase frontend latency before frames "
                          "are displayed.",
                          "Use a smaller client_snapshot_queue_capacity unless paced playback is "
                          "explicitly required.");
        }
        if (snapshotDropPolicy != "latest-only" && snapshotDropPolicy != "paced") {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "client_snapshot_drop_policy",
                          "Snapshot drop policy must be latest-only or paced.",
                          "Set client_snapshot_drop_policy to latest-only or paced.");
        }
        const std::string uiTheme = normalized(config.uiTheme);
        if (uiTheme != "light" && uiTheme != "dark") {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "ui_theme",
                          "Qt UI theme must be light or dark.", "Set ui_theme to light or dark.");
        }
        if (config.octreeSoftening <= 0.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "octree_softening",
                          "Softening octree_softening [m] must be strictly positive.",
                          "Set octree_softening above 0 m.");
        }
        else if (config.octreeSoftening < config.physicsMinSoftening) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Warning, "octree_softening",
                "Softening octree_softening [m] is below physics_min_softening [m] and will be "
                "clamped during force evaluation.",
                "Raise octree_softening [m] or lower physics_min_softening [m] intentionally.");
        }
        if (plan.config.velocityTemperature < 0.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "velocity_temperature",
                          "velocity_temperature [m/s] cannot be negative.",
                          "Set velocity_temperature to 0 m/s or a positive velocity scale.");
        }
        if (plan.config.particleTemperature < 0.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "particle_temperature",
                          "particle_temperature [K] cannot be negative.",
                          "Set particle_temperature to 0 K or a positive temperature.");
        }
        if (plan.config.thermalAmbientTemperature < 0.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "thermal_ambient_temperature",
                          "thermal_ambient_temperature [K] cannot be negative.",
                          "Set thermal_ambient_temperature to 0 K or above.");
        }
        if (!(plan.config.thermalSpecificHeat > 0.0f)) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "thermal_specific_heat",
                          "thermal_specific_heat [J/(kg*K)] must be strictly positive.",
                          "Set thermal_specific_heat to a value above 0 J/(kg*K).");
        }
        if (plan.config.thermalHeatingCoeff < 0.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "thermal_heating_coeff",
                          "thermal_heating_coeff must be non-negative.",
                          "Set thermal_heating_coeff to 0 or a positive value.");
        }
        if (plan.config.thermalRadiationCoeff < 0.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "thermal_radiation_coeff",
                          "thermal_radiation_coeff must be non-negative.",
                          "Set thermal_radiation_coeff to 0 or a positive value.");
        }
        if (requestedFileMode && plan.config.mode != "file") {
            addDiagnostic(
                ScenarioDiagnosticLevel::Error, "input_file",
                "File-based initialization was requested but no usable input_file was provided.",
                "Set input_file to an existing snapshot or switch init mode away from file.");
        }
        if (plan.config.mode != "file" && plan.config.particleMass <= 0.0f) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Error, "init_particle_mass",
                "Generated scenarios require a strictly positive particle mass.",
                "Raise init_particle_mass or choose a preset that computes masses automatically.");
        }
        if (plan.config.includeCentralBody && plan.config.centralMass <= 0.0f) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Error, "init_central_mass",
                "Central body mass must be strictly positive when the central body is enabled.",
                "Raise init_central_mass or disable the central body.");
        }
        if (plan.config.mode == "disk_orbit") {
            if (plan.config.diskMass <= 0.0f) {
                addDiagnostic(ScenarioDiagnosticLevel::Error, "init_disk_mass",
                              "Disk orbit scenarios require a strictly positive disk mass.",
                              "Raise init_disk_mass before running the scenario.");
            }
            if (plan.config.diskRadiusMax <= plan.config.diskRadiusMin) {
                addDiagnostic(ScenarioDiagnosticLevel::Error, "init_disk_radius",
                              "Disk radius max must be greater than disk radius min.",
                              "Increase init_disk_radius_max or lower init_disk_radius_min.");
            }
        }
        if ((plan.config.mode == "random_cloud" || plan.config.mode == "plummer_sphere") &&
            plan.config.cloudHalfExtent <= 0.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "init_cloud_half_extent",
                          "Cloud-based presets require a strictly positive spatial extent.",
                          "Set init_cloud_half_extent above 0.");
        }
        SimulationScenarioValidationPhysics::appendDiagnostics(config, plan.config, addDiagnostic);
        return report;
    }
};

ScenarioValidationReport SimulationScenarioValidation::evaluate(const SimulationConfig& config)
{
    return SimulationScenarioValidationLocal::evaluate(config);
}

std::string SimulationScenarioValidation::renderText(const ScenarioValidationReport& report)
{
    return SimulationScenarioValidationRender::render(report);
}
} // namespace grav_config
