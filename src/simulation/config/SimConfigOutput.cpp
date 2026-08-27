#include "simulation/config/SimConfigOutput.hpp"

#include "simulation/config/SimConfigDirective.hpp"
#include "simulation/config/SimConfigRun.hpp"
#include "simulation/config/SimConfigValue.hpp"

#include <array>
#include <new>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace blitzar_sim {

namespace {

[[nodiscard]] bool IsUsablePath(const std::filesystem::path& path) noexcept
{
    return !path.empty() && !path.filename().empty();
}

} // namespace

blitzar_status ResolveOutputDirectory(
    SimConfigOutput& output, const std::filesystem::path& config_directory) noexcept
{
    if (!output.enabled) {
        return BLITZAR_STATUS_OK;
    }

    try {
        const std::filesystem::path base =
            config_directory.empty() ? std::filesystem::path{"."} : config_directory;

        if (!output.directory.is_absolute()) {
            output.directory = base / output.directory;
        }

        output.directory = output.directory.lexically_normal();

        return IsUsablePath(output.directory) ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

blitzar_status ApplyOutputDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept
{
    constexpr std::array<std::string_view, 4> names{
        "directory", "every_steps", "write_initial", "write_final"};

    std::string_view directory_text;
    std::int64_t every_steps = 0;
    bool write_initial = false;
    bool write_final = false;

    if (!HasExactArguments(directive, names) ||
        !ReadConfigQuotedText(directive, "directory", directory_text) ||
        !ReadConfigInteger(directive, "every_steps", every_steps) ||
        !ReadConfigBoolean(directive, "write_initial", write_initial) ||
        !ReadConfigBoolean(directive, "write_final", write_final) || directory_text.empty() ||
        directory_text.find('\0') != std::string_view::npos || every_steps <= 0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        const std::filesystem::path directory{directory_text};

        if (!IsUsablePath(directory)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        config.output = {true, directory, every_steps, write_initial, write_final};

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
