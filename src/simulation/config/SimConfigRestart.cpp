#include "simulation/config/SimConfigRestart.hpp"

#include "simulation/config/SimConfigDirective.hpp"
#include "simulation/config/SimConfigRun.hpp"
#include "simulation/config/SimConfigValue.hpp"

#include <array>
#include <new>
#include <stdexcept>
#include <string_view>

namespace blitzar_sim {

namespace {

[[nodiscard]] bool IsUsablePath(const std::filesystem::path& path) noexcept
{
    return !path.empty() && !path.filename().empty();
}

} // namespace

blitzar_status ResolveRestartDirectory(
    SimConfigRestart& restart, const std::filesystem::path& config_directory) noexcept
{
    if (!restart.enabled) {
        return BLITZAR_STATUS_OK;
    }

    try {
        const std::filesystem::path base =
            config_directory.empty() ? std::filesystem::path{"."} : config_directory;

        if (!restart.directory.is_absolute()) {
            restart.directory = base / restart.directory;
        }

        restart.directory = restart.directory.lexically_normal();

        return IsUsablePath(restart.directory) ? BLITZAR_STATUS_OK
                                               : BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

blitzar_status ApplyRestartDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept
{
    constexpr std::array<std::string_view, 2> names{"directory", "step"};
    std::string_view directory_text;
    std::uint64_t step = 0;

    if (!HasExactArguments(directive, names) ||
        !ReadConfigQuotedText(directive, "directory", directory_text) ||
        !ReadConfigUnsigned(directive, "step", step) || directory_text.empty() ||
        directory_text.find('\0') != std::string_view::npos || step > SimConfigRun::MaxStateStep) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        const std::filesystem::path directory{directory_text};

        if (!IsUsablePath(directory)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        config.restart = {true, directory, step, 0.0};

        return BLITZAR_STATUS_OK;
    }
    catch (const std::filesystem::filesystem_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

} // namespace blitzar_sim
