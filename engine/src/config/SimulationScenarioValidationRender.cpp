/*
 * @file engine/src/config/SimulationScenarioValidationRender.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/SimulationScenarioValidationRender.hpp"
#include <sstream>

namespace bltzr_config {
std::string SimulationScenarioValidationRender::render(const ScenarioValidationReport& report)
{
    std::ostringstream out;
    if (report.errorCount == 0u && report.warningCount == 0u) {
        out << "[preflight] OK: no blocking scenario issues detected.";
        return out.str();
    }
    out << "[preflight] " << (report.validForRun ? "warnings" : "blocked") << ": "
        << report.errorCount << " error(s), " << report.warningCount << " warning(s)";
    for (const ScenarioDiagnostic& diagnostic : report.diagnostics) {
        out << "\n- " << (diagnostic.level == ScenarioDiagnosticLevel::Error ? "error" : "warning")
            << " [" << diagnostic.field << "] " << diagnostic.message;
        if (!diagnostic.action.empty()) {
            out << " Action: " << diagnostic.action;
        }
    }
    return out.str();
}
} // namespace bltzr_config
