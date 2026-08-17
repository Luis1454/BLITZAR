/*
 * @file engine/config/validation/scenario/CfgScenarioRuntime.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Responsibility-focused scenario validation rules.
 */

#include "CfgScenarioInternals.hpp"

#include "core/constants/FndConstants.hpp"

namespace bltzr_config {

void validateRuntimeConfiguration(const SimulationConfig& config, std::string_view sceneCopyAxis,
                                  std::string_view snapshotDropPolicy,
                                  ScenarioValidationContext& context)
{
    if (config.particleCount < 2u) {
        context.add(ScenarioDiagnosticLevel::Error, "particle_count",
                    "At least 2 particles are required to run the simulation.",
                    "Increase particle_count to 2 or more.");
    }
    else if (config.particleCount < 16u) {
        context.add(ScenarioDiagnosticLevel::Warning, "particle_count",
                    "Very low particle counts can make scenario behavior unrepresentative.",
                    "Use a larger particle_count for stable tuning and visualization.");
    }
    if (!(config.dt > 0.0f)) {
        context.add(ScenarioDiagnosticLevel::Error, "dt",
                    "Time step dt [s] must be strictly positive.", "Set dt to a value above 0 s.");
    }
    else if (config.dt > kMaxStableInteractiveDt) {
        context.add(ScenarioDiagnosticLevel::Warning, "dt",
                    "Large time steps dt [s] are clamped by the runtime to preserve stable "
                    "interactive orbits.",
                    "Use dt <= 10 s for interactive runs.");
    }
    if (config.substepTargetDt < 0.0f) {
        context.add(ScenarioDiagnosticLevel::Error, "substep_target_dt",
                    "Substep target dt cannot be negative.",
                    "Use 0 for auto mode or a positive target dt.");
    }
    else if (config.substepTargetDt > 0.0f && config.substepTargetDt > config.dt) {
        context.add(ScenarioDiagnosticLevel::Warning, "substep_target_dt",
                    "Configured substep target dt is larger than dt, so no finer subdivision "
                    "will happen.",
                    "Lower substep_target_dt or leave it at 0 for auto mode.");
    }
    if (config.maxSubsteps == 0u) {
        context.add(ScenarioDiagnosticLevel::Error, "max_substeps", "Maximum substeps cannot be 0.",
                    "Set max_substeps to at least 1.");
    }
    if (sceneCopyAxis != "x" && sceneCopyAxis != "y" && sceneCopyAxis != "z") {
        context.add(ScenarioDiagnosticLevel::Error, "scene_copy_axis",
                    "Scene rotation copy axis must be x, y, or z.",
                    "Set scene_copy_axis to x, y, or z.");
    }
    if (config.sceneRotationCopies == 0u || config.sceneRotationCopies > 256u) {
        context.add(ScenarioDiagnosticLevel::Error, "scene_rotation_copies",
                    "Scene rotation copies must be between 1 and 256.",
                    "Set scene_rotation_copies to a value in [1, 256].");
    }
    if (config.adaptiveTimeStepMaxLevel > 12u) {
        context.add(ScenarioDiagnosticLevel::Error, "adaptive_max_level",
                    "Adaptive time-step hierarchy is limited to 12 binary levels.",
                    "Set adaptive_max_level between 0 and 12.");
    }
    if (!(config.adaptiveTimeStepEta >= 0.01f && config.adaptiveTimeStepEta <= 1.0f)) {
        context.add(ScenarioDiagnosticLevel::Error, "adaptive_eta",
                    "Adaptive time-step eta must be between 0.01 and 1.0.",
                    "Set adaptive_eta to a value in [0.01, 1.0].");
    }
    if (config.clientSnapshotQueueCapacity == 0u) {
        context.add(ScenarioDiagnosticLevel::Error, "client_snapshot_queue_capacity",
                    "Snapshot queue capacity must be at least 1 frame.",
                    "Set client_snapshot_queue_capacity to 1 or more.");
    }
    else if (config.clientSnapshotQueueCapacity > 16u) {
        context.add(ScenarioDiagnosticLevel::Warning, "client_snapshot_queue_capacity",
                    "Large snapshot queue capacities increase frontend latency before frames "
                    "are displayed.",
                    "Use a smaller client_snapshot_queue_capacity unless paced playback is "
                    "explicitly required.");
    }
    if (snapshotDropPolicy != "latest-only" && snapshotDropPolicy != "paced") {
        context.add(ScenarioDiagnosticLevel::Error, "client_snapshot_drop_policy",
                    "Snapshot drop policy must be latest-only or paced.",
                    "Set client_snapshot_drop_policy to latest-only or paced.");
    }
    const std::string uiTheme = normalizeScenarioValue(config.uiTheme);
    if (uiTheme != "light" && uiTheme != "dark") {
        context.add(ScenarioDiagnosticLevel::Error, "ui_theme",
                    "Qt UI theme must be light or dark.", "Set ui_theme to light or dark.");
    }
    if (config.octreeSoftening <= 0.0f) {
        context.add(ScenarioDiagnosticLevel::Error, "octree_softening",
                    "Softening octree_softening [m] must be strictly positive.",
                    "Set octree_softening above 0 m.");
    }
    else if (config.octreeSoftening < config.physicsMinSoftening) {
        context.add(ScenarioDiagnosticLevel::Warning, "octree_softening",
                    "Softening octree_softening [m] is below physics_min_softening [m] and will be "
                    "clamped during force evaluation.",
                    "Raise octree_softening [m] or lower physics_min_softening [m] intentionally.");
    }
}

} // namespace bltzr_config
