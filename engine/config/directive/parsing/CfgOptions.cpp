/*
 * @file engine/config/directive/parsing/CfgOptions.cpp
 * @brief Directive aliases and configuration option application.
 */

#include "config/directive/parsing/CfgDirectiveInternals.hpp"

#include "config/core/configuration/CfgConfig.hpp"
#include "config/registry/runtime/CfgMain.hpp"

namespace bltzr_config {
struct DirectiveAlias {
    std::string_view directive;
    std::string_view argument;
    std::string_view iniKey;
};

constexpr DirectiveAlias kAliases[] = {
    {"simulation", "particles", "particle_count"},
    {"simulation", "profile", "simulation_profile"},
    {"performance", "profile", "performance_profile"},
    {"performance", "draw_cap", "client_particle_cap"},
    {"performance", "snapshot_ms", "snapshot_publish_period_ms"},
    {"performance", "energy_every", "energy_measure_every_steps"},
    {"performance", "sample_limit", "energy_sample_limit"},
    {"performance", "substep_target_dt", "substep_target_dt"},
    {"performance", "max_substeps", "max_substeps"},
    {"adaptive", "enabled", "adaptive_time_steps"},
    {"adaptive", "max_level", "adaptive_max_level"},
    {"adaptive", "eta", "adaptive_eta"},
    {"adaptive", "cost_guard", "adaptive_cost_guard"},
    {"octree", "theta", "octree_theta"},
    {"octree", "softening", "octree_softening"},
    {"octree", "criterion", "octree_opening_criterion"},
    {"octree", "theta_auto", "octree_theta_auto_tune"},
    {"octree", "theta_auto_min", "octree_theta_auto_min"},
    {"octree", "theta_auto_max", "octree_theta_auto_max"},
    {"octree", "leaf_capacity", "linear_octree_leaf_capacity"},
    {"octree", "cache_preference", "cuda_cache_preference"},
    {"treepm", "preset", "treepm_preset"},
    {"treepm", "enabled", "treepm_enabled"},
    {"treepm", "model", "treepm_model"},
    {"treepm", "layout", "treepm_layout"},
    {"treepm", "precision", "treepm_precision"},
    {"treepm", "assignment", "treepm_assignment"},
    {"treepm", "local_grid", "treepm_local_grid"},
    {"treepm", "grid_size", "treepm_grid_size"},
    {"treepm", "jacobi_iters", "treepm_jacobi_iterations"},
    {"treepm", "cutoff_factor", "treepm_cutoff_factor"},
    {"treepm", "max_local_neighbors", "treepm_max_local_neighbors"},
    {"treepm", "particle_limit", "treepm_particle_limit"},
    {"treepm", "dense_cell_threshold", "treepm_dense_cell_threshold"},
    {"treepm", "gravity_only_buffers", "treepm_gravity_only_buffers"},
    {"physics", "max_acceleration", "physics_max_acceleration"},
    {"physics", "min_softening", "physics_min_softening"},
    {"physics", "min_distance2", "physics_min_distance2"},
    {"physics", "min_theta", "physics_min_theta"},
    {"client", "zoom", "default_zoom"},
    {"client", "luminosity", "default_luminosity"},
    {"client", "theme", "ui_theme"},
    {"client", "ui_fps", "ui_fps_limit"},
    {"client", "command_timeout_ms", "client_remote_command_timeout_ms"},
    {"client", "status_timeout_ms", "client_remote_status_timeout_ms"},
    {"client", "snapshot_timeout_ms", "client_remote_snapshot_timeout_ms"},
    {"client", "snapshot_queue", "client_snapshot_queue_capacity"},
    {"client", "drop_policy", "client_snapshot_drop_policy"},
    {"export", "directory", "export_directory"},
    {"export", "format", "export_format"},
    {"scene", "style", "init_config_style"},
    {"scene", "preset", "preset_structure"},
    {"scene", "mode", "init_mode"},
    {"scene", "file", "input_file"},
    {"scene", "format", "input_format"},
    {"preset", "size", "preset_size"},
    {"preset", "temperature", "particle_temperature"},
    {"thermal", "ambient", "thermal_ambient_temperature"},
    {"thermal", "specific_heat", "thermal_specific_heat"},
    {"thermal", "heating", "thermal_heating_coeff"},
    {"thermal", "radiation", "thermal_radiation_coeff"},
    {"generation", "seed", "init_seed"},
    {"generation", "include_central_body", "init_include_central_body"},
    {"generation", "deterministic", "deterministic_mode"},
    {"central_body", "mass", "init_central_mass"},
    {"central_body", "x", "init_central_x"},
    {"central_body", "y", "init_central_y"},
    {"central_body", "z", "init_central_z"},
    {"central_body", "vx", "init_central_vx"},
    {"central_body", "vy", "init_central_vy"},
    {"central_body", "vz", "init_central_vz"},
    {"disk", "mass", "init_disk_mass"},
    {"disk", "radius_min", "init_disk_radius_min"},
    {"disk", "radius_max", "init_disk_radius_max"},
    {"disk", "thickness", "init_disk_thickness"},
    {"disk", "velocity_scale", "init_velocity_scale"},
    {"cloud", "half_extent", "init_cloud_half_extent"},
    {"cloud", "cube_half_extent", "init_cube_half_extent"},
    {"cloud", "sphere_radius", "init_sphere_radius"},
    {"cloud", "speed", "init_cloud_speed"},
    {"cloud", "particle_mass", "init_particle_mass"},
    {"cosmology", "enabled", "cosmology_enabled"},
    {"cosmology", "mode", "cosmology_mode"},
    {"cosmology", "geometry", "cosmology_geometry"},
    {"cosmology", "box_half_extent", "cosmology_box_half_extent"},
    {"cosmology", "sphere_radius", "cosmology_sphere_radius"},
    {"cosmology", "h0", "cosmology_h0"},
    {"cosmology", "omega_m", "cosmology_omega_m"},
    {"cosmology", "omega_lambda", "cosmology_omega_lambda"},
    {"cosmology", "omega_radiation", "cosmology_omega_radiation"},
    {"cosmology", "initial_scale_factor", "cosmology_initial_scale_factor"},
    {"cosmology", "perturbation", "cosmology_perturbation_amplitude"},
    {"cosmology", "peculiar_velocity", "cosmology_peculiar_velocity_scale"},
    {"cosmology", "mass_model", "cosmology_mass_model"},
    {"cosmology", "total_mass", "cosmology_total_mass"},
    {"transform", "offset_x", "scene_offset_x"},
    {"transform", "offset_y", "scene_offset_y"},
    {"transform", "offset_z", "scene_offset_z"},
    {"transform", "rotation_x", "scene_rotation_x"},
    {"transform", "rotation_y", "scene_rotation_y"},
    {"transform", "rotation_z", "scene_rotation_z"},
    {"transform", "copy_axis", "scene_copy_axis"},
    {"transform", "rotation_copies", "scene_rotation_copies"},
    {"transform", "mirror_x", "scene_mirror_x"},
    {"transform", "mirror_y", "scene_mirror_y"},
    {"transform", "mirror_z", "scene_mirror_z"},
    {"sph", "enabled", "sph_enabled"},
    {"sph", "smoothing_length", "sph_smoothing_length"},
    {"sph", "rest_density", "sph_rest_density"},
    {"sph", "gas_constant", "sph_gas_constant"},
    {"sph", "viscosity", "sph_viscosity"},
    {"sph", "max_acceleration", "sph_max_acceleration"},
    {"sph", "max_speed", "sph_max_speed"},
    {"energy", "every_steps", "energy_measure_every_steps"},
    {"energy", "sample_limit", "energy_sample_limit"},
    {"render", "culling", "render_culling_enabled"},
    {"render", "lod", "render_lod_enabled"},
    {"render", "lod_near", "render_lod_near_distance"},
    {"render", "lod_far", "render_lod_far_distance"},
};

constexpr std::string_view kKnownDirectives[] = {
    "simulation", "performance", "substeps",  "adaptive",  "octree",  "treepm",     "physics",
    "client",     "export",      "scene",     "preset",    "thermal", "generation", "central_body",
    "disk",       "cloud",       "cosmology", "transform", "sph",     "energy",     "render",
};

static std::string_view findIniKey(std::string_view directive, std::string_view argument)
{
    for (const DirectiveAlias& alias : kAliases) {
        if (alias.directive == directive && alias.argument == argument) {
            return alias.iniKey;
        }
    }
    for (const std::string_view known : kKnownDirectives) {
        if (known == directive) {
            return argument;
        }
    }
    return {};
}

static bool applyIniAlias(const DirectiveArgument& arg, std::string_view iniKey,
                          SimulationConfig& config, std::ostream& warnings)
{
    return applyIniOption(std::string(iniKey), arg.second, config, warnings);
}

bool applyDirectiveArgs(std::string_view directive, const DirectiveArguments& args,
                        SimulationConfig& config, std::ostream& warnings)
{
    if (directive == "object") {
        SceneObjectConfig object;
        for (const DirectiveArgument& arg : args) {
            if (!applySceneObjectArg(arg, object)) {
                warnings << "[config] unknown or invalid object argument: " << arg.first << "\n";
            }
        }
        config.scene.objects.push_back(std::move(object));
        return true;
    }
    if (directive == "modifier" || directive == "property") {
        applyLegacySceneProperty(args, config.scene, warnings);
        return true;
    }
    for (const DirectiveArgument& arg : args) {
        bool handled = false;
        if (directive == "scene" && arg.first == "kind") {
            handled = applyIniOption("preset_structure", arg.second, config, warnings) &&
                      applyIniOption("init_mode", arg.second, config, warnings);
        }
        else if (directive == "substeps") {
            if (arg.first == "target_dt") {
                handled = applyIniAlias(arg, "substep_target_dt", config, warnings);
            }
            else if (arg.first == "max") {
                handled = applyIniAlias(arg, "max_substeps", config, warnings);
            }
            else {
                handled = applyIniAlias(arg, arg.first, config, warnings);
            }
        }
        else {
            const std::string_view key = findIniKey(directive, arg.first);
            if (!key.empty()) {
                handled = applyIniAlias(arg, key, config, warnings);
            }
        }
        if (!handled) {
            warnings << "[config] unknown directive argument ignored: " << directive << '.'
                     << arg.first << "\n";
        }
    }
    return true;
}
} // namespace bltzr_config
