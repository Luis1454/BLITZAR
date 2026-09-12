#include "simulation/cfg/diagnostics/SimConfigDiagnostics.hpp"

#include "simulation/cfg/SimConfigValue.hpp"
#include "simulation/cfg/directive/SimConfigDirective.hpp"
#include "simulation/cfg/run/SimConfigRun.hpp"

#include <array>
#include <string_view>

namespace blitzar_sim {

blitzar_status ApplyDiagnosticsDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept
{
    constexpr std::array<std::string_view, 4> names{
        "every_steps", "energy", "momentum", "relative_error"};

    std::int64_t every_steps = 0;
    bool energy = false;
    bool momentum = false;
    bool relative_error = false;

    if (!HasExactArguments(directive, names) ||
        !ReadConfigInteger(directive, "every_steps", every_steps) ||
        !ReadConfigBoolean(directive, "energy", energy) ||
        !ReadConfigBoolean(directive, "momentum", momentum) ||
        !ReadConfigBoolean(directive, "relative_error", relative_error) || every_steps <= 0 ||
        (!energy && !momentum && !relative_error)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    config.diagnostics = {true, every_steps, energy, momentum, relative_error};

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_sim
