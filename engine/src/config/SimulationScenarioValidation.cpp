#include "config/SimulationScenarioValidation.hpp"

#include "config/SimulationConfig.hpp"
#include "server/SimulationInitConfig.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <utility>

namespace grav_config {

class SimulationScenarioValidationLocal final {
public:
    static ScenarioValidationReport evaluate(const SimulationConfig &config)
    {
        ScenarioValidationReport report;
        const auto addDiagnostic = [&report](ScenarioDiagnosticLevel level, std::string field, std::string message, std::string action) {
            ScenarioDiagnostic diagnostic;
            diagnostic.level = level;
            diagnostic.field = std::move(field);
            diagnostic.message = std::move(message);
            diagnostic.action = std::move(action);
            report.diagnostics.push_back(std::move(diagnostic));
            if (level == ScenarioDiagnosticLevel::Error) {
                report.errorCount += 1u;
                report.validForRun = false;
            } else {
                report.warningCount += 1u;
            }
        };

        std::ostringstream initLog;
        const ResolvedInitialStatePlan plan = resolveInitialStatePlan(config, initLog);
        std::istringstream logLines(initLog.str());
        for (std::string line; std::getline(logLines, line); ) {
            if (!line.empty()) {
                addDiagnostic(
                    ScenarioDiagnosticLevel::Warning,
                    "initial_state",
                    std::move(line),
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
        const bool requestedFileMode =
            (initStyle == "preset" && presetMode == "file")
            || (initStyle != "preset" && detailedMode == "file");

        if (config.particleCount < 2u) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Error,
                "particle_count",
                "At least 2 particles are required to run the simulation.",
                "Increase particle_count to 2 or more.");
        } else if (config.particleCount < 16u) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Warning,
                "particle_count",
                "Very low particle counts can make scenario behavior unrepresentative.",
                "Use a larger particle_count for stable tuning and visualization.");
        }

        if (!(config.dt > 0.0f)) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Error,
                "dt",
                "Time step must be strictly positive.",
                "Set dt to a value above 0.");
        } else if (config.dt > 0.05f) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Warning,
                "dt",
                "Large time steps increase the risk of unstable orbits and missed interactions.",
                "Reduce dt or enable a tighter substep_target_dt.");
        }

        if (config.substepTargetDt < 0.0f) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Error,
                "substep_target_dt",
                "Substep target dt cannot be negative.",
                "Use 0 for auto mode or a positive target dt.");
        } else if (config.substepTargetDt > 0.0f && config.substepTargetDt > config.dt) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Warning,
                "substep_target_dt",
                "Configured substep target dt is larger than dt, so no finer subdivision will happen.",
                "Lower substep_target_dt or leave it at 0 for auto mode.");
        }

        if (config.maxSubsteps == 0u) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Error,
                "max_substeps",
                "Maximum substeps cannot be 0.",
                "Set max_substeps to at least 1.");
        }

        if (config.octreeSoftening <= 0.0f) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Error,
                "octree_softening",
                "Softening must be strictly positive.",
                "Set octree_softening above 0.");
        } else if (config.octreeSoftening < config.physicsMinSoftening) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Warning,
                "octree_softening",
                "Softening is below physics_min_softening and will be clamped during force evaluation.",
                "Raise octree_softening or lower physics_min_softening intentionally.");
        }

        if (config.solver == "octree_gpu" || config.solver == "octree_cpu") {
            if (config.octreeTheta <= 0.0f) {
                addDiagnostic(
                    ScenarioDiagnosticLevel::Error,
                    "octree_theta",
                    "Octree theta must be strictly positive.",
                    "Set octree_theta above 0.");
            } else if (config.octreeTheta > 2.0f) {
                addDiagnostic(
                    ScenarioDiagnosticLevel::Warning,
                    "octree_theta",
                    "Large octree theta values can degrade force accuracy noticeably.",
                    "Lower octree_theta if you need more accurate trajectories.");
            }
        }

        if (config.sphEnabled) {
            if (config.sphSmoothingLength <= 0.0f || config.sphRestDensity <= 0.0f || config.sphGasConstant <= 0.0f) {
                addDiagnostic(
                    ScenarioDiagnosticLevel::Error,
                    "sph",
                    "SPH smoothing length, rest density, and gas constant must all be strictly positive.",
                    "Set positive SPH parameters before enabling SPH.");
            }
            if (config.sphViscosity < 0.0f) {
                addDiagnostic(
                    ScenarioDiagnosticLevel::Error,
                    "sph_viscosity",
                    "SPH viscosity cannot be negative.",
                    "Set sph_viscosity to 0 or a positive value.");
            }
            if (config.particleCount < 16u) {
                addDiagnostic(
                    ScenarioDiagnosticLevel::Warning,
                    "sph",
                    "SPH with very low particle counts usually produces poor density estimates.",
                    "Increase particle_count before using SPH diagnostics.");
            }
        }

        if (requestedFileMode && plan.config.mode != "file") {
            addDiagnostic(
                ScenarioDiagnosticLevel::Error,
                "input_file",
                "File-based initialization was requested but no usable input_file was provided.",
                "Set input_file to an existing snapshot or switch init mode away from file.");
        }

        if (plan.config.mode != "file" && plan.config.particleMass <= 0.0f) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Error,
                "init_particle_mass",
                "Generated scenarios require a strictly positive particle mass.",
                "Raise init_particle_mass or choose a preset that computes masses automatically.");
        }

        if (plan.config.includeCentralBody && plan.config.centralMass <= 0.0f) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Error,
                "init_central_mass",
                "Central body mass must be strictly positive when the central body is enabled.",
                "Raise init_central_mass or disable the central body.");
        }

        if (plan.config.mode == "disk_orbit") {
            if (plan.config.diskMass <= 0.0f) {
                addDiagnostic(
                    ScenarioDiagnosticLevel::Error,
                    "init_disk_mass",
                    "Disk orbit scenarios require a strictly positive disk mass.",
                    "Raise init_disk_mass before running the scenario.");
            }
            if (plan.config.diskRadiusMax <= plan.config.diskRadiusMin) {
                addDiagnostic(
                    ScenarioDiagnosticLevel::Error,
                    "init_disk_radius",
                    "Disk radius max must be greater than disk radius min.",
                    "Increase init_disk_radius_max or lower init_disk_radius_min.");
            }
        }

        if ((plan.config.mode == "random_cloud" || plan.config.mode == "plummer_sphere") && plan.config.cloudHalfExtent <= 0.0f) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Error,
                "init_cloud_half_extent",
                "Cloud-based presets require a strictly positive spatial extent.",
                "Set init_cloud_half_extent above 0.");
        }

        return report;
    }

    static std::string renderText(const ScenarioValidationReport &report)
    {
        std::ostringstream out;
        if (report.errorCount == 0u && report.warningCount == 0u) {
            out << "[preflight] OK: no blocking scenario issues detected.";
            return out.str();
        }

        out << "[preflight] "
            << (report.validForRun ? "warnings" : "blocked")
            << ": " << report.errorCount << " error(s), " << report.warningCount << " warning(s)";
        for (const ScenarioDiagnostic &diagnostic : report.diagnostics) {
            out << "\n- "
                << (diagnostic.level == ScenarioDiagnosticLevel::Error ? "error" : "warning")
                << " [" << diagnostic.field << "] "
                << diagnostic.message;
            if (!diagnostic.action.empty()) {
                out << " Action: " << diagnostic.action;
            }
        }
        return out.str();
    }
};

ScenarioValidationReport SimulationScenarioValidation::evaluate(const SimulationConfig &config)
{
    return SimulationScenarioValidationLocal::evaluate(config);
}

std::string SimulationScenarioValidation::renderText(const ScenarioValidationReport &report)
{
    return SimulationScenarioValidationLocal::renderText(report);
}

} // namespace grav_config
