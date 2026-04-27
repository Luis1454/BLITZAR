// File: engine/include/config/SimulationScenarioValidationRender.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONRENDER_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONRENDER_HPP_
#include "config/SimulationScenarioValidation.hpp"
#include <string>

namespace grav_config {
/// Description: Defines the SimulationScenarioValidationRender data or behavior contract.
class SimulationScenarioValidationRender final {
public:
    /// Description: Describes the render operation contract.
    static std::string render(const ScenarioValidationReport& report);
};
} // namespace grav_config
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONRENDER_HPP_
