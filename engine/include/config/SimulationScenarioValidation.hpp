#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATION_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATION_HPP_

#include "config/SimulationConfig.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace grav_config {

enum class ScenarioDiagnosticLevel {
    Warning,
    Error
};

struct ScenarioDiagnostic {
    ScenarioDiagnosticLevel level = ScenarioDiagnosticLevel::Warning;
    std::string field;
    std::string message;
    std::string action;
};

struct ScenarioValidationReport {
    std::vector<ScenarioDiagnostic> diagnostics;
    std::uint32_t warningCount = 0u;
    std::uint32_t errorCount = 0u;
    bool validForRun = true;
};

class SimulationScenarioValidation final {
public:
    static ScenarioValidationReport evaluate(const SimulationConfig &config);
    static std::string renderText(const ScenarioValidationReport &report);
};

} // namespace grav_config

#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONSCENARIOVALIDATION_HPP_
