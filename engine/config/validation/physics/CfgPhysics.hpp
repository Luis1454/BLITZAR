/*
 * @file engine/config/validation/physics/CfgPhysics.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONPHYSICS_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONPHYSICS_HPP_
#include "config/core/configuration/CfgConfig.hpp"
#include "config/validation/scenario/CfgScenario.hpp"
#include "server/simulation/configuration/SrvSimulationInitConfig.hpp"
#include <functional>
#include <string>

namespace bltzr_config {
void appendPhysicsDiagnostics(
    const SimulationConfig& config, const InitialStateConfig& resolvedInitConfig,
    const std::function<void(ScenarioDiagnosticLevel, std::string, std::string, std::string)>&
        addDiagnostic);
} // namespace bltzr_config
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONPHYSICS_HPP_
