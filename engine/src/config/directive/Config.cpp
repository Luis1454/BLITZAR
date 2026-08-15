/*
 * @file engine/src/config/SimulationConfigDirective.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/directive/Config.hpp"
#include "config/core/Config.hpp"
#include "config/registry/Main.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string_view>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bltzr_config {
static std::string trimDirective(std::string_view value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
                         return std::isspace(c) != 0;
                     }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

static std::string unquoteDirective(std::string value)
{
    if (value.size() >= 2u) {
        const char first = value.front();
        const char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return value.substr(1u, value.size() - 2u);
        }
    }
    return value;
}

static bool splitDirective(std::string_view raw, std::string& name,
                           std::vector<std::pair<std::string, std::string>>& args)
{
    const std::string stripped = trimDirective(raw);
    const std::size_t open = stripped.find('(');
    const std::size_t close = stripped.rfind(')');
    if (open == std::string::npos || close != stripped.size() - 1u || open == 0u) {
        return false;
    }
    name = trimDirective(stripped.substr(0u, open));
    const std::string_view body = std::string_view(stripped).substr(open + 1u, close - open - 1u);
    std::string token;
    char quote = '\0';
    for (char c : body) {
        if (quote != '\0') {
            if (c == quote) {
                quote = '\0';
            }
            token.push_back(c);
            continue;
        }
        if (c == '"' || c == '\'') {
            quote = c;
            token.push_back(c);
            continue;
        }
        if (c == ',') {
            const std::string entry = trimDirective(token);
            token.clear();
            if (entry.empty()) {
                continue;
            }
            const std::size_t eq = entry.find('=');
            if (eq == std::string::npos || eq == 0u) {
                return false;
            }
            args.emplace_back(trimDirective(entry.substr(0u, eq)),
                              unquoteDirective(trimDirective(entry.substr(eq + 1u))));
            continue;
        }
        token.push_back(c);
    }
    const std::string entry = trimDirective(token);
    if (entry.empty()) {
        return true;
    }
    const std::size_t eq = entry.find('=');
    if (eq == std::string::npos || eq == 0u) {
        return false;
    }
    args.emplace_back(trimDirective(entry.substr(0u, eq)),
                      unquoteDirective(trimDirective(entry.substr(eq + 1u))));
    return true;
}

static bool applyIniAlias(const std::pair<std::string, std::string>& arg, std::string_view iniKey,
                          SimulationConfig& config, std::ostream& warnings)
{
    return applyIniOption(std::string(iniKey), arg.second, config, warnings);
}

static bool parseSceneFloat(const std::string& raw, float& target)
{
    try {
        std::size_t consumed = 0u;
        const float value = std::stof(raw, &consumed);
        if (consumed != raw.size())
            return false;
        target = value;
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

static bool parseSceneUint(const std::string& raw, std::uint32_t& target)
{
    try {
        std::size_t consumed = 0u;
        const unsigned long value = std::stoul(raw, &consumed);
        if (consumed != raw.size() || value > 0xffffffffUL)
            return false;
        target = static_cast<std::uint32_t>(value);
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

static bool parseSceneBool(const std::string& raw, bool& target)
{
    std::string value = raw;
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (value == "true" || value == "1" || value == "on" || value == "yes") {
        target = true;
        return true;
    }
    if (value == "false" || value == "0" || value == "off" || value == "no") {
        target = false;
        return true;
    }
    return false;
}

static void addSceneObjectProperty(SceneObjectConfig& object, const std::string& property)
{
    if (property.empty() ||
        std::find(object.properties.begin(), object.properties.end(), property) !=
            object.properties.end())
        return;
    object.properties.push_back(property);
}

static void parseSceneObjectProperties(const std::string& raw, SceneObjectConfig& object)
{
    std::size_t begin = 0u;
    while (begin <= raw.size()) {
        const std::size_t end = raw.find(',', begin);
        addSceneObjectProperty(object, raw.substr(begin, end == std::string::npos
                                                           ? std::string::npos
                                                           : end - begin));
        if (end == std::string::npos)
            break;
        begin = end + 1u;
    }
}

static bool applySceneObjectArg(const std::pair<std::string, std::string>& arg,
                                SceneObjectConfig& object)
{
    const std::string& key = arg.first;
    const std::string& value = arg.second;
    if (key == "id") object.id = value;
    else if (key == "name") object.name = value;
    else if (key == "type") object.type = value;
    else if (key == "enabled") return parseSceneBool(value, object.enabled);
    else if (key == "include_central_body")
        return parseSceneBool(value, object.includeCentralBody);
    else if (key == "count" || key == "particle_count")
        return parseSceneUint(value, object.particleCount);
    else if (key == "seed") return parseSceneUint(value, object.seed);
    else if (key == "mass") return parseSceneFloat(value, object.mass);
    else if (key == "size") return parseSceneFloat(value, object.size);
    else if (key == "radius_min") return parseSceneFloat(value, object.radiusMin);
    else if (key == "radius_max") return parseSceneFloat(value, object.radiusMax);
    else if (key == "thickness") return parseSceneFloat(value, object.thickness);
    else if (key == "velocity_scale") return parseSceneFloat(value, object.velocityScale);
    else if (key == "speed") return parseSceneFloat(value, object.speed);
    else if (key == "particle_mass") return parseSceneFloat(value, object.particleMass);
    else if (key == "x") return parseSceneFloat(value, object.positionX);
    else if (key == "y") return parseSceneFloat(value, object.positionY);
    else if (key == "z") return parseSceneFloat(value, object.positionZ);
    else if (key == "vx") return parseSceneFloat(value, object.velocityX);
    else if (key == "vy") return parseSceneFloat(value, object.velocityY);
    else if (key == "vz") return parseSceneFloat(value, object.velocityZ);
    else if (key == "asset") return parseSceneBool(value, object.isAsset);
    else if (key == "asset_id") object.assetId = value;
    else if (key == "property") addSceneObjectProperty(object, value);
    else if (key == "properties") parseSceneObjectProperties(value, object);
    else if (key == "offset_x") return parseSceneFloat(value, object.offsetX);
    else if (key == "offset_y") return parseSceneFloat(value, object.offsetY);
    else if (key == "offset_z") return parseSceneFloat(value, object.offsetZ);
    else if (key == "rotation_x") return parseSceneFloat(value, object.rotationX);
    else if (key == "rotation_y") return parseSceneFloat(value, object.rotationY);
    else if (key == "rotation_z") return parseSceneFloat(value, object.rotationZ);
    else if (key == "copy_axis" || key == "axis") object.axis = value;
    else if (key == "rotation_copies" || key == "copies")
        return parseSceneUint(value, object.copies);
    else if (key == "mirror_x") return parseSceneBool(value, object.mirrorX);
    else if (key == "mirror_y") return parseSceneBool(value, object.mirrorY);
    else if (key == "mirror_z") return parseSceneBool(value, object.mirrorZ);
    else if (key == "pivot") object.pivot = value;
    else if (key == "pivot_x") return parseSceneFloat(value, object.pivotX);
    else if (key == "pivot_y") return parseSceneFloat(value, object.pivotY);
    else if (key == "pivot_z") return parseSceneFloat(value, object.pivotZ);
    else if (key == "distribution") object.distribution = value;
    else if (key == "particle_size") return parseSceneFloat(value, object.particleSize);
    else if (key == "particle_height") return parseSceneFloat(value, object.particleHeight);
    else if (key == "particle_speed") return parseSceneFloat(value, object.particleSpeed);
    else if (key == "emitter_object_id") object.emitterObjectId = value;
    else if (key == "target_asset_id" || key == "instance_object_id") object.targetAssetId = value;
    else return false;
    return true;
}

struct LegacySceneProperty {
    std::string type = "transform";
    bool enabled = true;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float offsetZ = 0.0f;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    std::string axis = "z";
    std::uint32_t copies = 1u;
    bool mirrorX = false;
    bool mirrorY = false;
    bool mirrorZ = false;
    std::string pivot = "world";
    float pivotX = 0.0f;
    float pivotY = 0.0f;
    float pivotZ = 0.0f;
    std::string distribution = "uniform_sphere";
    std::uint32_t particleCount = 0u;
    std::uint32_t seed = 42u;
    float particleSize = 1.0f;
    float particleHeight = 1.0f;
    float particleMass = 0.001f;
    float particleSpeed = 0.0f;
    std::string emitterObjectId;
    std::string instanceObjectId;
};

static bool applyLegacyPropertyArg(const std::pair<std::string, std::string>& arg,
                                   LegacySceneProperty& property);

static void applyLegacySceneProperty(const std::vector<std::pair<std::string, std::string>>& args,
                                     SceneConfig& scene, std::ostream& warnings)
{
    if (scene.objects.empty()) {
        warnings << "[config] property ignored because no scene object exists\n";
        return;
    }
    LegacySceneProperty property;
    for (const auto& arg : args) {
        if (!applyLegacyPropertyArg(arg, property)) {
            warnings << "[config] unknown or invalid property argument: " << arg.first << "\n";
        }
    }
    SceneObjectConfig& object = scene.objects.back();
    if (property.type == "particle_system") {
        SceneObjectConfig system;
        std::uint32_t systemSequence = 1u;
        bool hasMatchingId = true;
        while (hasMatchingId) {
            system.id = object.id + "_system_" + std::to_string(systemSequence++);
            hasMatchingId = std::any_of(scene.objects.begin(), scene.objects.end(),
                                        [&system](const SceneObjectConfig& candidate) {
                                            return candidate.id == system.id;
                                        });
        }
        system.name = object.name + " Particle System";
        system.type = "particle_system";
        system.enabled = property.enabled;
        system.particleCount = property.particleCount;
        system.seed = property.seed;
        system.particleMass = property.particleMass;
        system.particleSize = property.particleSize;
        system.particleHeight = property.particleHeight;
        system.particleSpeed = property.particleSpeed;
        system.distribution = property.distribution;
        system.emitterObjectId = property.emitterObjectId.empty()
                                     ? object.id
                                     : property.emitterObjectId;
        system.targetAssetId = property.instanceObjectId;
        scene.objects.push_back(std::move(system));
        return;
    }
    object.offsetX += property.offsetX;
    object.offsetY += property.offsetY;
    object.offsetZ += property.offsetZ;
    object.rotationX += property.rotationX;
    object.rotationY += property.rotationY;
    object.rotationZ += property.rotationZ;
    object.axis = property.axis;
    object.copies = std::min<std::uint32_t>(256u,
                                            object.copies *
                                                std::max<std::uint32_t>(1u, property.copies));
    object.mirrorX = object.mirrorX || property.mirrorX;
    object.mirrorY = object.mirrorY || property.mirrorY;
    object.mirrorZ = object.mirrorZ || property.mirrorZ;
    if (property.pivot != "world") {
        object.pivot = property.pivot;
        object.pivotX = property.pivotX;
        object.pivotY = property.pivotY;
        object.pivotZ = property.pivotZ;
    }
}

static bool applyLegacyPropertyArg(const std::pair<std::string, std::string>& arg,
                                   LegacySceneProperty& property)
{
    const std::string& key = arg.first;
    const std::string& value = arg.second;
    if (key == "type") property.type = value;
    else if (key == "enabled") return parseSceneBool(value, property.enabled);
    else if (key == "offset_x") return parseSceneFloat(value, property.offsetX);
    else if (key == "offset_y") return parseSceneFloat(value, property.offsetY);
    else if (key == "offset_z") return parseSceneFloat(value, property.offsetZ);
    else if (key == "rotation_x") return parseSceneFloat(value, property.rotationX);
    else if (key == "rotation_y") return parseSceneFloat(value, property.rotationY);
    else if (key == "rotation_z") return parseSceneFloat(value, property.rotationZ);
    else if (key == "axis" || key == "copy_axis") property.axis = value;
    else if (key == "copies" || key == "rotation_copies")
        return parseSceneUint(value, property.copies);
    else if (key == "mirror_x") return parseSceneBool(value, property.mirrorX);
    else if (key == "mirror_y") return parseSceneBool(value, property.mirrorY);
    else if (key == "mirror_z") return parseSceneBool(value, property.mirrorZ);
    else if (key == "pivot") property.pivot = value;
    else if (key == "pivot_x") return parseSceneFloat(value, property.pivotX);
    else if (key == "pivot_y") return parseSceneFloat(value, property.pivotY);
    else if (key == "pivot_z") return parseSceneFloat(value, property.pivotZ);
    else if (key == "distribution") property.distribution = value;
    else if (key == "particle_count") return parseSceneUint(value, property.particleCount);
    else if (key == "seed") return parseSceneUint(value, property.seed);
    else if (key == "particle_size" || key == "size")
        return parseSceneFloat(value, property.particleSize);
    else if (key == "particle_height" || key == "height")
        return parseSceneFloat(value, property.particleHeight);
    else if (key == "particle_mass") return parseSceneFloat(value, property.particleMass);
    else if (key == "particle_speed") return parseSceneFloat(value, property.particleSpeed);
    else if (key == "emitter_object_id") property.emitterObjectId = value;
    else if (key == "instance_object_id") property.instanceObjectId = value;
    else return false;
    return true;
}

static bool applyDirectiveArgs(std::string_view directive,
                               const std::vector<std::pair<std::string, std::string>>& args,
                               SimulationConfig& config, std::ostream& warnings)
{
    if (directive == "object") {
        SceneObjectConfig object;
        for (const auto& arg : args) {
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
    for (const auto& arg : args) {
        bool handled = false;
        if (directive == "simulation") {
            const std::string iniKey = arg.first == "particles"    ? "particle_count"
                                       : arg.first == "profile"    ? "simulation_profile"
                                                                    : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "performance") {
            const std::string iniKey = arg.first == "profile"        ? "performance_profile"
                                       : arg.first == "draw_cap"     ? "client_particle_cap"
                                       : arg.first == "snapshot_ms"  ? "snapshot_publish_period_ms"
                                       : arg.first == "energy_every" ? "energy_measure_every_steps"
                                       : arg.first == "sample_limit" ? "energy_sample_limit"
                                       : arg.first == "substep_target_dt" ? "substep_target_dt"
                                       : arg.first == "max_substeps"      ? "max_substeps"
                                                                          : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "substeps") {
            handled = applyIniAlias(arg,
                                    arg.first == "target_dt"
                                        ? "substep_target_dt"
                                        : (arg.first == "max" ? "max_substeps" : arg.first),
                                    config, warnings);
        }
        else if (directive == "adaptive") {
            const std::string iniKey = arg.first == "enabled"   ? "adaptive_time_steps"
                                       : arg.first == "max_level" ? "adaptive_max_level"
                                       : arg.first == "eta"       ? "adaptive_eta"
                                       : arg.first == "cost_guard" ? "adaptive_cost_guard"
                                                                   : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "octree") {
            const std::string iniKey = arg.first == "theta"            ? "octree_theta"
                                       : arg.first == "softening"      ? "octree_softening"
                                       : arg.first == "criterion"      ? "octree_opening_criterion"
                                       : arg.first == "theta_auto"     ? "octree_theta_auto_tune"
                                       : arg.first == "theta_auto_min" ? "octree_theta_auto_min"
                                       : arg.first == "theta_auto_max" ? "octree_theta_auto_max"
                                       : arg.first == "leaf_capacity"
                                           ? "linear_octree_leaf_capacity"
                                       : arg.first == "cache_preference" ? "cuda_cache_preference"
                                                                         : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "treepm") {
            const std::string iniKey =
                arg.first == "preset"                 ? "treepm_preset"
                : arg.first == "enabled"              ? "treepm_enabled"
                : arg.first == "model"                ? "treepm_model"
                : arg.first == "layout"               ? "treepm_layout"
                : arg.first == "precision"            ? "treepm_precision"
                : arg.first == "assignment"           ? "treepm_assignment"
                : arg.first == "local_grid"           ? "treepm_local_grid"
                : arg.first == "grid_size"            ? "treepm_grid_size"
                : arg.first == "jacobi_iters"         ? "treepm_jacobi_iterations"
                : arg.first == "cutoff_factor"        ? "treepm_cutoff_factor"
                : arg.first == "max_local_neighbors"  ? "treepm_max_local_neighbors"
                : arg.first == "particle_limit"       ? "treepm_particle_limit"
                : arg.first == "dense_cell_threshold" ? "treepm_dense_cell_threshold"
                : arg.first == "gravity_only_buffers" ? "treepm_gravity_only_buffers"
                                                      : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "physics") {
            const std::string iniKey = arg.first == "max_acceleration" ? "physics_max_acceleration"
                                       : arg.first == "min_softening"  ? "physics_min_softening"
                                       : arg.first == "min_distance2"  ? "physics_min_distance2"
                                       : arg.first == "min_theta"      ? "physics_min_theta"
                                                                       : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "client") {
            const std::string iniKey =
                arg.first == "zoom"                  ? "default_zoom"
                : arg.first == "luminosity"          ? "default_luminosity"
                : arg.first == "theme"               ? "ui_theme"
                : arg.first == "ui_fps"              ? "ui_fps_limit"
                : arg.first == "command_timeout_ms"  ? "client_remote_command_timeout_ms"
                : arg.first == "status_timeout_ms"   ? "client_remote_status_timeout_ms"
                : arg.first == "snapshot_timeout_ms" ? "client_remote_snapshot_timeout_ms"
                : arg.first == "snapshot_queue"      ? "client_snapshot_queue_capacity"
                : arg.first == "drop_policy"         ? "client_snapshot_drop_policy"
                                                     : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "export") {
            if (arg.first == "directory") {
                handled = applyIniAlias(arg, "export_directory", config, warnings);
            }
            else if (arg.first == "format") {
                handled = applyIniAlias(arg, "export_format", config, warnings);
            }
            else {
                handled = applyIniAlias(arg, arg.first, config, warnings);
            }
        }
        else if (directive == "scene") {
            if (arg.first == "kind") {
                handled = applyIniOption("preset_structure", arg.second, config, warnings) &&
                          applyIniOption("init_mode", arg.second, config, warnings);
            }
            else {
                const std::string iniKey = arg.first == "style"    ? "init_config_style"
                                           : arg.first == "preset" ? "preset_structure"
                                           : arg.first == "mode"   ? "init_mode"
                                           : arg.first == "file"   ? "input_file"
                                           : arg.first == "format" ? "input_format"
                                                                   : arg.first;
                handled = applyIniAlias(arg, iniKey, config, warnings);
            }
        }
        else if (directive == "preset") {
            const std::string iniKey = arg.first == "size"          ? "preset_size"
                                       : arg.first == "temperature" ? "particle_temperature"
                                                                    : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "thermal") {
            const std::string iniKey = arg.first == "ambient" ? "thermal_ambient_temperature"
                                       : arg.first == "specific_heat" ? "thermal_specific_heat"
                                       : arg.first == "heating"       ? "thermal_heating_coeff"
                                       : arg.first == "radiation"     ? "thermal_radiation_coeff"
                                                                      : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "generation") {
            const std::string iniKey = arg.first == "seed" ? "init_seed"
                                       : arg.first == "include_central_body"
                                           ? "init_include_central_body"
                                       : arg.first == "deterministic" ? "deterministic_mode"
                                                                      : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "central_body") {
            const std::string iniKey = arg.first == "mass" ? "init_central_mass"
                                       : arg.first == "x"  ? "init_central_x"
                                       : arg.first == "y"  ? "init_central_y"
                                       : arg.first == "z"  ? "init_central_z"
                                       : arg.first == "vx" ? "init_central_vx"
                                       : arg.first == "vy" ? "init_central_vy"
                                       : arg.first == "vz" ? "init_central_vz"
                                                           : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "disk") {
            const std::string iniKey = arg.first == "mass"             ? "init_disk_mass"
                                       : arg.first == "radius_min"     ? "init_disk_radius_min"
                                       : arg.first == "radius_max"     ? "init_disk_radius_max"
                                       : arg.first == "thickness"      ? "init_disk_thickness"
                                       : arg.first == "velocity_scale" ? "init_velocity_scale"
                                                                       : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "cloud") {
            const std::string iniKey = arg.first == "half_extent"        ? "init_cloud_half_extent"
                                       : arg.first == "cube_half_extent" ? "init_cube_half_extent"
                                       : arg.first == "sphere_radius"    ? "init_sphere_radius"
                                       : arg.first == "speed"            ? "init_cloud_speed"
                                       : arg.first == "particle_mass"    ? "init_particle_mass"
                                                                         : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "cosmology") {
            const std::string iniKey = arg.first == "enabled" ? "cosmology_enabled"
                                       : arg.first == "mode" ? "cosmology_mode"
                                       : arg.first == "geometry" ? "cosmology_geometry"
                                       : arg.first == "box_half_extent" ? "cosmology_box_half_extent"
                                       : arg.first == "sphere_radius" ? "cosmology_sphere_radius"
                                       : arg.first == "h0" ? "cosmology_h0"
                                       : arg.first == "omega_m" ? "cosmology_omega_m"
                                       : arg.first == "omega_lambda" ? "cosmology_omega_lambda"
                                       : arg.first == "omega_radiation" ? "cosmology_omega_radiation"
                                       : arg.first == "initial_scale_factor"
                                           ? "cosmology_initial_scale_factor"
                                       : arg.first == "perturbation" ? "cosmology_perturbation_amplitude"
                                       : arg.first == "peculiar_velocity"
                                           ? "cosmology_peculiar_velocity_scale"
                                       : arg.first == "mass_model" ? "cosmology_mass_model"
                                       : arg.first == "total_mass" ? "cosmology_total_mass"
                                       : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "transform") {
            const std::string iniKey =
                arg.first == "offset_x" ? "scene_offset_x"
                : arg.first == "offset_y" ? "scene_offset_y"
                : arg.first == "offset_z" ? "scene_offset_z"
                : arg.first == "rotation_x" ? "scene_rotation_x"
                : arg.first == "rotation_y" ? "scene_rotation_y"
                : arg.first == "rotation_z" ? "scene_rotation_z"
                : arg.first == "copy_axis" ? "scene_copy_axis"
                : arg.first == "rotation_copies" ? "scene_rotation_copies"
                : arg.first == "mirror_x" ? "scene_mirror_x"
                : arg.first == "mirror_y" ? "scene_mirror_y"
                : arg.first == "mirror_z" ? "scene_mirror_z"
                : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "sph") {
            const std::string iniKey = arg.first == "enabled"            ? "sph_enabled"
                                       : arg.first == "smoothing_length" ? "sph_smoothing_length"
                                       : arg.first == "rest_density"     ? "sph_rest_density"
                                       : arg.first == "gas_constant"     ? "sph_gas_constant"
                                       : arg.first == "viscosity"        ? "sph_viscosity"
                                       : arg.first == "max_acceleration" ? "sph_max_acceleration"
                                       : arg.first == "max_speed"        ? "sph_max_speed"
                                                                         : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "energy") {
            const std::string iniKey = arg.first == "every_steps"    ? "energy_measure_every_steps"
                                       : arg.first == "sample_limit" ? "energy_sample_limit"
                                                                     : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        else if (directive == "render") {
            const std::string iniKey = arg.first == "culling"    ? "render_culling_enabled"
                                       : arg.first == "lod"      ? "render_lod_enabled"
                                       : arg.first == "lod_near" ? "render_lod_near_distance"
                                       : arg.first == "lod_far"  ? "render_lod_far_distance"
                                                                 : arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        if (!handled) {
            warnings << "[config] unknown directive argument ignored: " << directive << '.'
                     << arg.first << "\n";
        }
    }
    return true;
}

bool SimulationConfigDirective::applyLine(const std::string& line, SimulationConfig& config,
                                          std::ostream& warnings)
{
    std::string directive;
    std::vector<std::pair<std::string, std::string>> args;
    if (!splitDirective(line, directive, args)) {
        return false;
    }
    if (directive.empty()) {
        warnings << "[config] invalid directive ignored: " << line << "\n";
        return true;
    }
    return applyDirectiveArgs(directive, args, config, warnings);
}
} // namespace bltzr_config
