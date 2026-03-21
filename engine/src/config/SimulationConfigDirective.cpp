#include "config/SimulationConfigDirective.hpp"

#include "config/SimulationConfig.hpp"
#include "config/SimulationOptionRegistry.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <utility>
#include <vector>

namespace grav_config {

static std::string trimDirective(std::string_view value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
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

static bool splitDirective(std::string_view raw, std::string &name, std::vector<std::pair<std::string, std::string>> &args)
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
            args.emplace_back(trimDirective(entry.substr(0u, eq)), unquoteDirective(trimDirective(entry.substr(eq + 1u))));
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
    args.emplace_back(trimDirective(entry.substr(0u, eq)), unquoteDirective(trimDirective(entry.substr(eq + 1u))));
    return true;
}

static bool applyIniAlias(
    const std::pair<std::string, std::string> &arg,
    std::string_view iniKey,
    SimulationConfig &config,
    std::ostream &warnings)
{
    return applyIniOption(std::string(iniKey), arg.second, config, warnings);
}

static bool applyDirectiveArgs(
    std::string_view directive,
    const std::vector<std::pair<std::string, std::string>> &args,
    SimulationConfig &config,
    std::ostream &warnings)
{
    for (const auto &arg : args) {
        bool handled = false;
        if (directive == "simulation") {
            handled = applyIniAlias(arg, arg.first == "particles" ? "particle_count" : arg.first, config, warnings);
        } else if (directive == "performance") {
            const std::string iniKey =
                arg.first == "profile" ? "performance_profile" :
                arg.first == "draw_cap" ? "client_particle_cap" :
                arg.first == "snapshot_ms" ? "snapshot_publish_period_ms" :
                arg.first == "energy_every" ? "energy_measure_every_steps" :
                arg.first == "sample_limit" ? "energy_sample_limit" :
                arg.first == "substep_target_dt" ? "substep_target_dt" :
                arg.first == "max_substeps" ? "max_substeps" :
                arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        } else if (directive == "substeps") {
            handled = applyIniAlias(arg, arg.first == "target_dt" ? "substep_target_dt" : (arg.first == "max" ? "max_substeps" : arg.first), config, warnings);
        } else if (directive == "octree") {
            handled = applyIniAlias(arg, arg.first == "theta" ? "octree_theta" : (arg.first == "softening" ? "octree_softening" : arg.first), config, warnings);
        } else if (directive == "client") {
            const std::string iniKey =
                arg.first == "zoom" ? "default_zoom" :
                arg.first == "luminosity" ? "default_luminosity" :
                arg.first == "ui_fps" ? "ui_fps_limit" :
                arg.first == "command_timeout_ms" ? "client_remote_command_timeout_ms" :
                arg.first == "status_timeout_ms" ? "client_remote_status_timeout_ms" :
                arg.first == "snapshot_timeout_ms" ? "client_remote_snapshot_timeout_ms" :
                arg.first == "snapshot_queue" ? "client_snapshot_queue_capacity" :
                arg.first == "drop_policy" ? "client_snapshot_drop_policy" :
                arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        } else if (directive == "export") {
            if (arg.first == "directory") {
                handled = applyIniAlias(arg, "export_directory", config, warnings);
            } else if (arg.first == "format") {
                handled = applyIniAlias(arg, "export_format", config, warnings);
            } else {
                handled = applyIniAlias(arg, arg.first, config, warnings);
            }
        } else if (directive == "scene") {
            if (arg.first == "kind") {
                handled = applyIniOption("preset_structure", arg.second, config, warnings)
                    && applyIniOption("init_mode", arg.second, config, warnings);
            } else {
                const std::string iniKey =
                    arg.first == "style" ? "init_config_style" :
                    arg.first == "preset" ? "preset_structure" :
                    arg.first == "mode" ? "init_mode" :
                    arg.first == "file" ? "input_file" :
                    arg.first == "format" ? "input_format" :
                    arg.first;
                handled = applyIniAlias(arg, iniKey, config, warnings);
            }
        } else if (directive == "preset") {
            const std::string iniKey =
                arg.first == "size" ? "preset_size" :
                arg.first == "temperature" ? "particle_temperature" :
                arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        } else if (directive == "thermal") {
            const std::string iniKey =
                arg.first == "ambient" ? "thermal_ambient_temperature" :
                arg.first == "specific_heat" ? "thermal_specific_heat" :
                arg.first == "heating" ? "thermal_heating_coeff" :
                arg.first == "radiation" ? "thermal_radiation_coeff" :
                arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        } else if (directive == "generation") {
            const std::string iniKey =
                arg.first == "seed" ? "init_seed" :
                arg.first == "include_central_body" ? "init_include_central_body" :
                arg.first == "deterministic" ? "deterministic_mode" :
                arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        } else if (directive == "central_body") {
            const std::string iniKey =
                arg.first == "mass" ? "init_central_mass" :
                arg.first == "x" ? "init_central_x" :
                arg.first == "y" ? "init_central_y" :
                arg.first == "z" ? "init_central_z" :
                arg.first == "vx" ? "init_central_vx" :
                arg.first == "vy" ? "init_central_vy" :
                arg.first == "vz" ? "init_central_vz" :
                arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        } else if (directive == "disk") {
            const std::string iniKey =
                arg.first == "mass" ? "init_disk_mass" :
                arg.first == "radius_min" ? "init_disk_radius_min" :
                arg.first == "radius_max" ? "init_disk_radius_max" :
                arg.first == "thickness" ? "init_disk_thickness" :
                arg.first == "velocity_scale" ? "init_velocity_scale" :
                arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        } else if (directive == "cloud") {
            const std::string iniKey =
                arg.first == "half_extent" ? "init_cloud_half_extent" :
                arg.first == "speed" ? "init_cloud_speed" :
                arg.first == "particle_mass" ? "init_particle_mass" :
                arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        } else if (directive == "sph") {
            const std::string iniKey =
                arg.first == "enabled" ? "sph_enabled" :
                arg.first == "smoothing_length" ? "sph_smoothing_length" :
                arg.first == "rest_density" ? "sph_rest_density" :
                arg.first == "gas_constant" ? "sph_gas_constant" :
                arg.first == "viscosity" ? "sph_viscosity" :
                arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        } else if (directive == "energy") {
            const std::string iniKey =
                arg.first == "every_steps" ? "energy_measure_every_steps" :
                arg.first == "sample_limit" ? "energy_sample_limit" :
                arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        } else if (directive == "render") {
            const std::string iniKey =
                arg.first == "culling" ? "render_culling_enabled" :
                arg.first == "lod" ? "render_lod_enabled" :
                arg.first == "lod_near" ? "render_lod_near_distance" :
                arg.first == "lod_far" ? "render_lod_far_distance" :
                arg.first;
            handled = applyIniAlias(arg, iniKey, config, warnings);
        }
        if (!handled) {
            warnings << "[config] unknown directive argument ignored: " << directive << '.' << arg.first << "\n";
        }
    }
    return true;
}

bool SimulationConfigDirective::applyLine(const std::string &line, SimulationConfig &config, std::ostream &warnings)
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

} // namespace grav_config
