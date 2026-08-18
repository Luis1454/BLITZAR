/*
 * @file engine/config/validation/scenario/CfgScenarioCosmology.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Responsibility-focused scenario validation rules.
 */

#include "CfgScenarioInternals.hpp"

namespace bltzr_config {

void validateCosmology(const SimulationConfig& config, const ResolvedInitialStatePlan& plan,
                       ScenarioValidationContext& context)
{
    if (plan.config.mode == "cosmology") {
        const std::string geometry = plan.config.cosmology.geometry;
        if (geometry != "sphere" && geometry != "cube") {
            context.add(ScenarioDiagnosticLevel::Error, "cosmology_geometry",
                        "Cosmology geometry must be sphere or cube.",
                        "Set cosmology geometry to sphere or cube.");
        }
        if (plan.config.cosmology.mode == "comoving" && geometry != "cube") {
            context.add(ScenarioDiagnosticLevel::Error, "cosmology_comoving_geometry",
                        "Comoving cosmology requires a periodic cube.",
                        "Set cosmology geometry=cube or use mode=expanding_preview.");
        }
        if (plan.config.cosmology.mode == "comoving" &&
            (config.treePmModel != "pm_only" || !config.treePmEnabled)) {
            context.add(ScenarioDiagnosticLevel::Error, "cosmology_comoving_solver",
                        "Comoving cosmology requires the periodic TreePM PM-only solver.",
                        "Set treepm enabled=true and model=pm_only.");
        }
        if (plan.config.cosmology.mode == "comoving" && config.solver != "octree_cpu") {
            context.add(ScenarioDiagnosticLevel::Error, "cosmology_comoving_backend",
                        "The CUDA comoving PM path is not qualified for GUI execution.",
                        "Use solver=octree_cpu until the CUDA qualification is complete.");
        }
        if (plan.config.cosmology.mode == "comoving" && config.sphEnabled) {
            context.add(ScenarioDiagnosticLevel::Error, "cosmology_comoving_sph",
                        "Comoving cosmology does not combine with SPH in this solver.",
                        "Disable SPH for the PM-only cosmology mode.");
        }
        if (plan.config.cosmology.mode == "comoving" && config.integrator != "leapfrog") {
            context.add(ScenarioDiagnosticLevel::Error, "cosmology_comoving_integrator",
                        "Comoving cosmology requires the KDK leapfrog integrator.",
                        "Set integrator=leapfrog.");
        }
        if (plan.config.cosmology.hubbleH0 <= 0.0f) {
            context.add(ScenarioDiagnosticLevel::Error, "cosmology_h0",
                        "Cosmology H0 must be strictly positive.",
                        "Set cosmology h0 in simulation inverse-time units.");
        }
        if (plan.config.cosmology.initialScaleFactor <= 0.0f) {
            context.add(
                ScenarioDiagnosticLevel::Error, "cosmology_initial_scale_factor",
                "Initial scale factor must be strictly positive.",
                "Use a positive starting scale factor; values above 1 continue an expanded model.");
        }
        if (plan.config.cosmology.boxHalfExtent <= 0.0f ||
            plan.config.cosmology.sphereRadius <= 0.0f) {
            context.add(ScenarioDiagnosticLevel::Error, "cosmology_extent",
                        "Cosmology spatial extents must be strictly positive.",
                        "Set both cosmology extents above 0.");
        }
    }
}

} // namespace bltzr_config
