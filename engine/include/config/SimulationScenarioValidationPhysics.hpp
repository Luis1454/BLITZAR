/*
 * @file engine/include/config/SimulationScenarioValidationPhysics.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONPHYSICS_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONPHYSICS_HPP_
#include "config/SimulationConfig.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include "server/SimulationInitConfig.hpp"
#include <functional>
#include <string>

namespace bltzr_config {
class SimulationScenarioValidationPhysics final {
public:
    static void appendDiagnostics(
        const SimulationConfig& config, const InitialStateConfig& resolvedInitConfig,
        const std::function<void(ScenarioDiagnosticLevel, std::string, std::string, std::string)>&
            addDiagnostic);
};
} // namespace bltzr_config
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONPHYSICS_HPP_
