#include "config/SimulationScenarioValidation.hpp"

#include "config/SimulationConfig.hpp"

#include <gtest/gtest.h>

#include <string>

namespace grav_test_config_scenario_validation_edges {

static bool hasField(const grav_config::ScenarioValidationReport &report, const std::string &field)
{
    for (const grav_config::ScenarioDiagnostic &diagnostic : report.diagnostics) {
        if (diagnostic.field == field) {
            return true;
        }
    }
    return false;
}

static std::size_t countField(const grav_config::ScenarioValidationReport &report, const std::string &field)
{
    std::size_t count = 0u;
    for (const grav_config::ScenarioDiagnostic &diagnostic : report.diagnostics) {
        if (diagnostic.field == field) {
            count += 1u;
        }
    }
    return count;
}

TEST(ScenarioValidationEdgesTest, TST_UNT_CONF_074_NegativeSubstepTargetDtBlocksRun)
{
    SimulationConfig config;
    config.substepTargetDt = -0.1f;

    const grav_config::ScenarioValidationReport report = grav_config::SimulationScenarioValidation::evaluate(config);

    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "substep_target_dt"));
}

TEST(ScenarioValidationEdgesTest, TST_UNT_CONF_075_InvalidInitStyleAddsInitialStateWarning)
{
    SimulationConfig config;
    config.initConfigStyle = "nonsense";

    const grav_config::ScenarioValidationReport report = grav_config::SimulationScenarioValidation::evaluate(config);

    EXPECT_TRUE(report.validForRun);
    EXPECT_GT(report.warningCount, 0u);
    EXPECT_TRUE(hasField(report, "initial_state"));
}

TEST(ScenarioValidationEdgesTest, TST_UNT_CONF_076_OctreeThetaAboveTwoProducesWarning)
{
    SimulationConfig config;
    config.solver = "octree_cpu";
    config.octreeOpeningCriterion = "com";
    config.octreeTheta = 2.5f;
    config.octreeThetaAutoMin = 0.4f;
    config.octreeThetaAutoMax = 1.6f;

    const grav_config::ScenarioValidationReport report = grav_config::SimulationScenarioValidation::evaluate(config);

    EXPECT_TRUE(report.validForRun);
    EXPECT_GT(report.warningCount, 0u);
    EXPECT_TRUE(hasField(report, "octree_theta"));
}

TEST(ScenarioValidationEdgesTest, TST_UNT_CONF_077_StepTravelWarningsTriggerForSofteningAndLengthScale)
{
    SimulationConfig config;
    config.dt = 0.5f;
    config.velocityTemperature = 100.0f;
    config.octreeSoftening = 2.0f;
    config.presetSize = 10.0f;

    const grav_config::ScenarioValidationReport report = grav_config::SimulationScenarioValidation::evaluate(config);

    EXPECT_TRUE(report.validForRun);
    EXPECT_GE(countField(report, "dt"), 2u);
}

TEST(ScenarioValidationEdgesTest, TST_UNT_CONF_078_RenderTextWarningModeOmitsActionWhenEmpty)
{
    grav_config::ScenarioValidationReport report;
    report.validForRun = true;
    report.warningCount = 1u;

    grav_config::ScenarioDiagnostic warning;
    warning.level = grav_config::ScenarioDiagnosticLevel::Warning;
    warning.field = "dt";
    warning.message = "example warning";
    warning.action.clear();
    report.diagnostics.push_back(warning);

    const std::string rendered = grav_config::SimulationScenarioValidation::renderText(report);
    EXPECT_NE(rendered.find("[preflight] warnings"), std::string::npos);
    EXPECT_NE(rendered.find("example warning"), std::string::npos);
    EXPECT_EQ(rendered.find("Action:"), std::string::npos);
}

} // namespace grav_test_config_scenario_validation_edges
