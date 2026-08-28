#ifndef BLITZAR_SIMULATION_CONFIG_SIM_CONFIG_DIRECTIVE_HPP
#define BLITZAR_SIMULATION_CONFIG_SIM_CONFIG_DIRECTIVE_HPP

#include "simulation/config/SimConfigFile.hpp"
#include "simulation/config/SimConfigRun.hpp"

namespace blitzar_sim {

[[nodiscard]] blitzar_status ApplySimulationDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept;

[[nodiscard]] blitzar_status ApplyGravityDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept;

[[nodiscard]] blitzar_status ApplyUnitsDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept;

[[nodiscard]] blitzar_status ApplyBarnesHutDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept;

[[nodiscard]] blitzar_status ApplyGenerationDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept;

[[nodiscard]] blitzar_status ApplyRunDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept;

[[nodiscard]] blitzar_status ApplyOutputDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept;

[[nodiscard]] blitzar_status ApplyDiagnosticsDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept;

[[nodiscard]] blitzar_status ApplyRestartDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept;

} // namespace blitzar_sim

#endif
