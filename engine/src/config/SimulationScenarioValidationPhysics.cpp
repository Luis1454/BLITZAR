#include "config/SimulationScenarioValidationPhysics.hpp"
#include "config/SimulationModes.hpp"
#include <algorithm>
#include <cmath>
#include <string>
namespace grav_config {
void SimulationScenarioValidationPhysics::appendDiagnostics(
    const SimulationConfig& config, const InitialStateConfig& resolvedInitConfig,
    const std::function<void(ScenarioDiagnosticLevel, std::string, std::string, std::string)>&
        addDiagnostic)
{
    if (!(config.physicsMaxAcceleration > 0.0f)) {
        addDiagnostic(ScenarioDiagnosticLevel::Error, "physics_max_acceleration",
                      "physics_max_acceleration [m/s^2] must be strictly positive.",
                      "Set physics_max_acceleration above 0 m/s^2.");
    }
    if (!(config.physicsMinSoftening > 0.0f)) {
        addDiagnostic(ScenarioDiagnosticLevel::Error, "physics_min_softening",
                      "physics_min_softening [m] must be strictly positive.",
                      "Set physics_min_softening above 0 m.");
    }
    if (!(config.physicsMinDistance2 > 0.0f)) {
        addDiagnostic(ScenarioDiagnosticLevel::Error, "physics_min_distance2",
                      "physics_min_distance2 [m^2] must be strictly positive.",
                      "Set physics_min_distance2 above 0 m^2.");
    }
    else if (config.octreeSoftening > 0.0f &&
             config.physicsMinDistance2 > config.octreeSoftening * config.octreeSoftening) {
        addDiagnostic(
            ScenarioDiagnosticLevel::Warning, "physics_min_distance2",
            "physics_min_distance2 [m^2] exceeds octree_softening^2 [m^2], so near-field clamping "
            "will dominate before softening.",
            "Lower physics_min_distance2 [m^2] or raise octree_softening [m] intentionally.");
    }
    if (config.solver == "octree_gpu" || config.solver == "octree_cpu") {
        std::string canonicalCriterion;
        if (!grav_modes::normalizeOctreeOpeningCriterion(config.octreeOpeningCriterion,
                                                         canonicalCriterion)) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "octree_opening_criterion",
                          "Octree opening criterion must be com or bounds.",
                          "Set octree_opening_criterion to com or bounds.");
        }
        if (config.octreeTheta <= 0.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "octree_theta",
                          "Octree theta must be strictly positive.", "Set octree_theta above 0.");
        }
        else if (config.octreeTheta > 2.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Warning, "octree_theta",
                          "Large octree theta values can degrade force accuracy noticeably.",
                          "Lower octree_theta if you need more accurate trajectories.");
        }
        if (config.octreeThetaAutoMin <= 0.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "octree_theta_auto_min",
                          "Octree theta auto-tune minimum must be strictly positive.",
                          "Set octree_theta_auto_min above 0.");
        }
        if (config.octreeThetaAutoMax <= 0.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "octree_theta_auto_max",
                          "Octree theta auto-tune maximum must be strictly positive.",
                          "Set octree_theta_auto_max above 0.");
        }
        else if (config.octreeThetaAutoMax < config.octreeThetaAutoMin) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "octree_theta_auto_max",
                          "Octree theta auto-tune maximum must be greater than or equal to "
                          "octree_theta_auto_min.",
                          "Raise octree_theta_auto_max or lower octree_theta_auto_min.");
        }
    }
    if (config.sphEnabled) {
        if (config.sphSmoothingLength <= 0.0f || config.sphRestDensity <= 0.0f ||
            config.sphGasConstant <= 0.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "sph",
                          "SPH smoothing length, rest density, and gas constant must all be "
                          "strictly positive.",
                          "Set positive SPH parameters before enabling SPH.");
        }
        if (config.sphViscosity < 0.0f) {
            addDiagnostic(ScenarioDiagnosticLevel::Error, "sph_viscosity",
                          "SPH viscosity cannot be negative.",
                          "Set sph_viscosity to 0 or a positive value.");
        }
        if (config.particleCount < 16u) {
            addDiagnostic(
                ScenarioDiagnosticLevel::Warning, "sph",
                "SPH with very low particle counts usually produces poor density estimates.",
                "Increase particle_count before using SPH diagnostics.");
        }
    }
    const float maxConfiguredSpeed = std::max(
        {std::fabs(resolvedInitConfig.velocityScale), std::fabs(resolvedInitConfig.cloudSpeed),
         std::fabs(resolvedInitConfig.velocityTemperature), std::fabs(resolvedInitConfig.centralVx),
         std::fabs(resolvedInitConfig.centralVy), std::fabs(resolvedInitConfig.centralVz)});
    const float characteristicLength =
        std::max({config.octreeSoftening, config.presetSize, resolvedInitConfig.diskRadiusMax,
                  resolvedInitConfig.cloudHalfExtent});
    const float stepTravel = config.dt * maxConfiguredSpeed;
    if (stepTravel > 0.0f && config.octreeSoftening > 0.0f && stepTravel > config.octreeSoftening) {
        addDiagnostic(
            ScenarioDiagnosticLevel::Warning, "dt",
            "Single-step travel dt [s] * velocity [m/s] exceeds octree_softening [m].",
            "Lower dt [s], lower configured velocity [m/s], or raise octree_softening [m].");
    }
    if (stepTravel > 0.0f && characteristicLength > 0.0f && stepTravel > characteristicLength) {
        addDiagnostic(ScenarioDiagnosticLevel::Warning, "dt",
                      "Single-step travel dt [s] * velocity [m/s] exceeds the configured scenario "
                      "length scale [m].",
                      "Lower dt [s] or reduce the configured velocity scale [m/s] before running.");
    }
}
} // namespace grav_config
