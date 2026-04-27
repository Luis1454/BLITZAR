// File: engine/include/config/SimulationScenarioValidation.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATION_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATION_HPP_
#include <cstdint>
#include <string>
#include <vector>
/// Description: Defines the SimulationConfig data or behavior contract.
struct SimulationConfig;
namespace grav_config {
/// Description: Enumerates the supported ScenarioDiagnosticLevel values.
enum class ScenarioDiagnosticLevel { Warning, Error };
/// Description: Defines the ScenarioDiagnostic data or behavior contract.
struct ScenarioDiagnostic {
    ScenarioDiagnosticLevel level = ScenarioDiagnosticLevel::Warning;
    std::string field;
    std::string message;
    std::string action;
};
/// Description: Defines the ScenarioValidationReport data or behavior contract.
struct ScenarioValidationReport {
    std::vector<ScenarioDiagnostic> diagnostics;
    std::uint32_t warningCount = 0u;
    std::uint32_t errorCount = 0u;
    bool validForRun = true;
};
/// Description: Defines the SimulationScenarioValidation data or behavior contract.
class SimulationScenarioValidation final {
public:
    /// Description: Executes the evaluate operation.
    static ScenarioValidationReport evaluate(const SimulationConfig& config);
    /// Description: Executes the renderText operation.
    static std::string renderText(const ScenarioValidationReport& report);
};
} // namespace grav_config
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATION_HPP_
