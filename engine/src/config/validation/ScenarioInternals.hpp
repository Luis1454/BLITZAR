/*
 * @file engine/src/config/validation/ScenarioInternals.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Private coordination contracts for scenario validation.
 */

#ifndef BLITZAR_ENGINE_SRC_CONFIG_VALIDATION_SCENARIOINTERNALS_HPP_
#define BLITZAR_ENGINE_SRC_CONFIG_VALIDATION_SCENARIOINTERNALS_HPP_

#include "config/core/Config.hpp"
#include "config/validation/Scenario.hpp"
#include "server/SimulationInitConfig.hpp"

#include <string>
#include <string_view>

namespace bltzr_config {

struct ScenarioValidationContext final {
    explicit ScenarioValidationContext(ScenarioValidationReport& targetReport) noexcept;

    void add(ScenarioDiagnosticLevel level, std::string field, std::string message,
             std::string action);

    ScenarioValidationReport& report;
};

std::string normalizeScenarioValue(std::string value);

void validateSceneObjects(const SimulationConfig& config, ScenarioValidationContext& context);

void validateRuntimeConfiguration(const SimulationConfig& config, std::string_view sceneCopyAxis,
                                  std::string_view snapshotDropPolicy,
                                  ScenarioValidationContext& context);

void validateInitialState(const ResolvedInitialStatePlan& plan, bool requestedFileMode,
                          ScenarioValidationContext& context);

void validateCosmology(const SimulationConfig& config, const ResolvedInitialStatePlan& plan,
                       ScenarioValidationContext& context);

} // namespace bltzr_config

#endif // BLITZAR_ENGINE_SRC_CONFIG_VALIDATION_SCENARIOINTERNALS_HPP_
