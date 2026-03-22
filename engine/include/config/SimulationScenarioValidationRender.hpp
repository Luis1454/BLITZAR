#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONRENDER_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONRENDER_HPP_

#include <string>

#include "config/SimulationScenarioValidation.hpp"

namespace grav_config {
class SimulationScenarioValidationRender final {
public:
    static std::string render(const ScenarioValidationReport &report);
};
} // namespace grav_config

#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONRENDER_HPP_
