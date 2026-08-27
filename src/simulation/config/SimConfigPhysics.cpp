#include "simulation/config/SimConfigDirective.hpp"
#include "simulation/config/SimConfigValue.hpp"

#include <array>
#include <cmath>
#include <string_view>

namespace blitzar_sim {

blitzar_status ApplyGravityDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept
{
    constexpr std::array<std::string_view, 2> names{"gravitational_constant", "softening"};
    double gravitational_constant = 0.0;
    double softening = 0.0;

    if (!HasExactArguments(directive, names) ||
        !ReadConfigReal(directive, "gravitational_constant", gravitational_constant) ||
        !ReadConfigReal(directive, "softening", softening) ||
        !std::isfinite(gravitational_constant) || gravitational_constant <= 0.0 ||
        !std::isfinite(softening) || softening < 0.0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    config.gravitational_constant = gravitational_constant;
    config.softening = softening;

    return BLITZAR_STATUS_OK;
}

blitzar_status ApplyUnitsDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept
{
    constexpr std::array<std::string_view, 3> names{"length_scale", "mass_scale", "time_scale"};
    double length_scale = 0.0;
    double mass_scale = 0.0;
    double time_scale = 0.0;

    if (!HasExactArguments(directive, names) ||
        !ReadConfigReal(directive, "length_scale", length_scale) ||
        !ReadConfigReal(directive, "mass_scale", mass_scale) ||
        !ReadConfigReal(directive, "time_scale", time_scale) || !std::isfinite(length_scale) ||
        length_scale <= 0.0 || !std::isfinite(mass_scale) || mass_scale <= 0.0 ||
        !std::isfinite(time_scale) || time_scale <= 0.0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    config.length_scale = length_scale;
    config.mass_scale = mass_scale;
    config.time_scale = time_scale;

    return BLITZAR_STATUS_OK;
}

blitzar_status ApplyBarnesHutDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept
{
    constexpr std::array<std::string_view, 5> names{
        "opening_angle", "max_particles", "max_cells", "leaf_capacity", "max_depth"};

    double opening_angle = 0.0;
    std::int64_t max_particles = 0;
    std::int64_t max_cells = 0;
    std::int64_t leaf_capacity = 0;
    std::int64_t max_depth = 0;

    if (!HasExactArguments(directive, names) ||
        !ReadConfigReal(directive, "opening_angle", opening_angle) ||
        !ReadConfigInteger(directive, "max_particles", max_particles) ||
        !ReadConfigInteger(directive, "max_cells", max_cells) ||
        !ReadConfigInteger(directive, "leaf_capacity", leaf_capacity) ||
        !ReadConfigInteger(directive, "max_depth", max_depth) || !std::isfinite(opening_angle) ||
        opening_angle < 0.0 || max_particles <= 0 || max_cells <= 0 || leaf_capacity <= 0 ||
        max_depth <= 0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    config.barnes_hut = {opening_angle, max_particles, max_cells, leaf_capacity, max_depth};

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_sim
