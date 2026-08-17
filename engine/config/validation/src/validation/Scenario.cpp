/*
 * @file engine/config/validation/src/validation/Scenario.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Scenario validation orchestration and diagnostic report construction.
 */

#include "ScenarioInternals.hpp"

#include "validation/Physics.hpp"
#include "validation/Render.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>

namespace bltzr_config {

ScenarioValidationContext::ScenarioValidationContext(
    ScenarioValidationReport& targetReport) noexcept
    : report(targetReport)
{
}

void ScenarioValidationContext::add(ScenarioDiagnosticLevel level, std::string field,
                                    std::string message, std::string action)
{
    ScenarioDiagnostic diagnostic;
    diagnostic.level = level;
    diagnostic.field = std::move(field);
    diagnostic.message = std::move(message);
    diagnostic.action = std::move(action);
    report.diagnostics.push_back(std::move(diagnostic));
    if (level == ScenarioDiagnosticLevel::Error) {
        report.errorCount += 1u;
        report.validForRun = false;
    }
    else {
        report.warningCount += 1u;
    }
}

std::string normalizeScenarioValue(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

ScenarioValidationReport SimulationScenarioValidation::evaluate(const SimulationConfig& config)
{
    ScenarioValidationReport report;
    ScenarioValidationContext context(report);
    std::ostringstream initLog;
    const ResolvedInitialStatePlan plan = resolveInitialStatePlan(config, initLog);
    std::istringstream logLines(initLog.str());
    for (std::string line; std::getline(logLines, line);) {
        if (!line.empty()) {
            context.add(ScenarioDiagnosticLevel::Warning, "initial_state", std::move(line),
                        "Review init mode, preset selection, and input file settings.");
        }
    }

    const std::string initStyle = normalizeScenarioValue(config.initConfigStyle);
    const std::string presetMode = normalizeScenarioValue(config.presetStructure);
    const std::string detailedMode = normalizeScenarioValue(config.initMode);
    const std::string snapshotDropPolicy = normalizeScenarioValue(config.clientSnapshotDropPolicy);
    const std::string sceneCopyAxis = normalizeScenarioValue(config.sceneCopyAxis);
    const bool requestedFileMode = (initStyle == "preset" && presetMode == "file") ||
                                   (initStyle != "preset" && detailedMode == "file");

    validateSceneObjects(config, context);
    validateRuntimeConfiguration(config, sceneCopyAxis, snapshotDropPolicy, context);
    validateInitialState(plan, requestedFileMode, context);
    validateCosmology(config, plan, context);
    appendPhysicsDiagnostics(config, plan.config,
                             [&context](ScenarioDiagnosticLevel level, std::string field,
                                        std::string message, std::string action) {
                                 context.add(level, std::move(field), std::move(message),
                                             std::move(action));
                             });
    return report;
}

std::string SimulationScenarioValidation::renderText(const ScenarioValidationReport& report)
{
    return renderValidationReport(report);
}

} // namespace bltzr_config
