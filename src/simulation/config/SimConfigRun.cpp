#include "simulation/config/SimConfigRun.hpp"

#include "simulation/config/SimConfigDirective.hpp"
#include "simulation/config/SimConfigValue.hpp"

#include <array>
#include <cstddef>
#include <new>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace blitzar_sim {

blitzar_status ApplyGenerationDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept
{
    constexpr std::array<std::string_view, 2> names{"seed", "deterministic"};
    std::uint64_t seed = 0;
    bool deterministic = false;

    if (!HasExactArguments(directive, names) || !ReadConfigUnsigned(directive, "seed", seed) ||
        !ReadConfigBoolean(directive, "deterministic", deterministic)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (!deterministic) {
        return BLITZAR_STATUS_UNSUPPORTED;
    }

    config.seed = seed;
    config.deterministic = deterministic;

    return BLITZAR_STATUS_OK;
}

blitzar_status ApplyRunDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept
{
    constexpr std::array<std::string_view, 1> names{"steps"};
    std::int64_t steps = 0;

    if (!HasExactArguments(directive, names) || !ReadConfigInteger(directive, "steps", steps) ||
        steps <= 0 || steps > SimConfigRun::MaxSteps) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    config.steps = steps;

    return BLITZAR_STATUS_OK;
}

namespace {

[[nodiscard]] bool FindDirectiveIndex(std::string_view name, std::size_t& index) noexcept
{
    if (name == "simulation") {
        index = 0;
    }
    else if (name == "gravity") {
        index = 1;
    }
    else if (name == "units") {
        index = 2;
    }
    else if (name == "generation") {
        index = 3;
    }
    else if (name == "barnes_hut") {
        index = 4;
    }
    else if (name == "run") {
        index = 5;
    }
    else if (name == "output") {
        index = 6;
    }
    else if (name == "diagnostics") {
        index = 7;
    }
    else {
        return false;
    }

    return true;
}

[[nodiscard]] blitzar_status ApplyDirectiveValues(
    const SimConfigFile::Directive& directive, std::size_t index, SimConfigRun& config) noexcept
{
    switch (index) {
    case 0:

        return ApplySimulationDirective(directive, config);

    case 1:

        return ApplyGravityDirective(directive, config);

    case 2:

        return ApplyUnitsDirective(directive, config);

    case 3:

        return ApplyGenerationDirective(directive, config);

    case 4:

        return ApplyBarnesHutDirective(directive, config);

    case 5:

        return ApplyRunDirective(directive, config);

    case 6:

        return ApplyOutputDirective(directive, config);

    case 7:

        return ApplyDiagnosticsDirective(directive, config);

    default:

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
}

[[nodiscard]] blitzar_status ApplyDirective(const SimConfigFile::Directive& directive,
    std::array<bool, 8>& seen, SimConfigRun& config) noexcept
{
    std::size_t index = 0;

    if (!FindDirectiveIndex(directive.name, index)) {
        return BLITZAR_STATUS_UNSUPPORTED;
    }

    if (seen[index]) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    seen[index] = true;

    return ApplyDirectiveValues(directive, index, config);
}

} // namespace

blitzar_status BuildRunConfig(const SimConfigFile& source, SimConfigRun& destination) noexcept
{
    try {
        const std::filesystem::path config_directory{"."};

        return BuildRunConfig(source, config_directory, destination);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

blitzar_status BuildRunConfig(const SimConfigFile& source,
    const std::filesystem::path& config_directory, SimConfigRun& destination) noexcept
{
    try {
        SimConfigRun candidate;
        std::array<bool, 8> seen{};

        for (const SimConfigFile::Directive& directive : source.directives) {
            const blitzar_status status = ApplyDirective(directive, seen, candidate);

            if (status != BLITZAR_STATUS_OK) {
                return status;
            }
        }

        if (!seen[0] || !seen[1] || !seen[2] || !seen[3]) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        if (!seen[4]) {
            candidate.barnes_hut = {
                0.5, candidate.particle_count, candidate.particle_count * 8 + 1, 8, 32};
        }

        if (candidate.barnes_hut.max_particles < candidate.particle_count) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const blitzar_status path_status =
            ResolveOutputDirectory(candidate.output, config_directory);

        if (path_status != BLITZAR_STATUS_OK) {
            return path_status;
        }

        destination = std::move(candidate);

        return BLITZAR_STATUS_OK;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

} // namespace blitzar_sim
