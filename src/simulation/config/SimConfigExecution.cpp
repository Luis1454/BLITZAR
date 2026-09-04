#include "core/CoreExecution.hpp"
#include "simulation/config/SimConfigDirective.hpp"
#include "simulation/config/SimConfigValue.hpp"

#include <array>
#include <string_view>

namespace blitzar_sim {

blitzar_status ApplyExecutionDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept
{
    constexpr std::array<std::string_view, 1> names{"mode"};
    std::string_view mode_text;
    blitzar_core::ExecutionMode mode{};

    if (!HasExactArguments(directive, names) || !ReadConfigText(directive, "mode", mode_text) ||
        !blitzar_core::ParseExecutionMode(mode_text, mode)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    config.execution = mode == blitzar_core::ExecutionMode::Strict
                           ? blitzar_core::ExecutionSettings::Strict(config.seed)
                           : blitzar_core::ExecutionSettings::Fast(config.seed);

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_sim
