/*
 * @file engine/include/config/validation/Physics.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONPHYSICS_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONPHYSICS_HPP_
#include "config/core/Config.hpp"
#include "config/validation/Scenario.hpp"
#include "server/SimulationInitConfig.hpp"
#include <functional>
#include <string>

namespace bltzr_config {
void appendPhysicsDiagnostics(
    const SimulationConfig& config, const InitialStateConfig& resolvedInitConfig,
    const std::function<void(ScenarioDiagnosticLevel, std::string, std::string, std::string)>&
        addDiagnostic);
} // namespace bltzr_config
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONPHYSICS_HPP_
