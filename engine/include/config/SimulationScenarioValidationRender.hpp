/*
 * @file engine/include/config/SimulationScenarioValidationRender.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONRENDER_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONRENDER_HPP_
#include "config/SimulationScenarioValidation.hpp"
#include <string>

namespace bltzr_config {
class SimulationScenarioValidationRender final {
public:
    static std::string render(const ScenarioValidationReport& report);
};
} // namespace bltzr_config
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONRENDER_HPP_
