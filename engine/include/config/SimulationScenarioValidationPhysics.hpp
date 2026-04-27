// File: engine/include/config/SimulationScenarioValidationPhysics.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONPHYSICS_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONPHYSICS_HPP_
#include "config/SimulationConfig.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include "server/SimulationInitConfig.hpp"
#include <functional>
#include <string>

namespace grav_config {
/// Description: Defines the SimulationScenarioValidationPhysics data or behavior contract.
class SimulationScenarioValidationPhysics final {
public:
    /// Description: Describes the append diagnostics operation contract.
    static void appendDiagnostics(
        const SimulationConfig& config, const InitialStateConfig& resolvedInitConfig,
        const std::function<void(ScenarioDiagnosticLevel, std::string, std::string, std::string)>&
            addDiagnostic);
};
} // namespace grav_config
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATIONPHYSICS_HPP_
