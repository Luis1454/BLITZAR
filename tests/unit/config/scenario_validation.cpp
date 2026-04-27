// File: tests/unit/config/scenario_validation.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "config/SimulationConfig.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include <gtest/gtest.h>
#include <string>
namespace grav_test_config_scenario_validation {
/// Description: Executes the hasField operation.
static bool hasField(const grav_config::ScenarioValidationReport& report, const std::string& field)
{
    for (const grav_config::ScenarioDiagnostic& diagnostic : report.diagnostics)
        if (diagnostic.field == field)
            return true;
    return false;
}
/// Description: Executes the TEST operation.
TEST(ScenarioValidationTest, TST_UNT_CONF_055_DefaultConfigurationIsRunnable)
{
    const SimulationConfig config;
    const grav_config::ScenarioValidationReport report =
        /// Description: Executes the evaluate operation.
        grav_config::SimulationScenarioValidation::evaluate(config);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(report.validForRun);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(report.errorCount, 0u);
    const std::string rendered = grav_config::SimulationScenarioValidation::renderText(report);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(rendered.find("[preflight] OK"), std::string::npos);
}
/// Description: Executes the TEST operation.
TEST(ScenarioValidationTest, TST_UNT_CONF_056_InvalidCoreFieldsBlockRun)
{
    SimulationConfig config;
    config.particleCount = 1u;
    config.dt = 0.0f;
    config.maxSubsteps = 0u;
    config.clientSnapshotQueueCapacity = 0u;
    config.clientSnapshotDropPolicy = "unexpected";
    config.uiTheme = "neon";
    config.octreeSoftening = 0.0f;
    const grav_config::ScenarioValidationReport report =
        /// Description: Executes the evaluate operation.
        grav_config::SimulationScenarioValidation::evaluate(config);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(report.validForRun);
    /// Description: Executes the EXPECT_GT operation.
    EXPECT_GT(report.errorCount, 0u);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "particle_count"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "dt"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "max_substeps"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "client_snapshot_queue_capacity"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "client_snapshot_drop_policy"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "ui_theme"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "octree_softening"));
}
/// Description: Executes the TEST operation.
TEST(ScenarioValidationTest, TST_UNT_CONF_057_WarningPathsRemainRunnable)
{
    SimulationConfig config;
    config.particleCount = 8u;
    config.dt = 0.1f;
    config.substepTargetDt = 0.2f;
    config.clientSnapshotQueueCapacity = 32u;
    config.octreeSoftening = 1e-5f;
    config.physicsMinSoftening = 1e-3f;
    const grav_config::ScenarioValidationReport report =
        /// Description: Executes the evaluate operation.
        grav_config::SimulationScenarioValidation::evaluate(config);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(report.validForRun);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(report.errorCount, 0u);
    /// Description: Executes the EXPECT_GT operation.
    EXPECT_GT(report.warningCount, 0u);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "particle_count"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "dt"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "substep_target_dt"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "client_snapshot_queue_capacity"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "octree_softening"));
}
/// Description: Executes the TEST operation.
TEST(ScenarioValidationTest, TST_UNT_CONF_058_FileModeWithoutInputProducesError)
{
    SimulationConfig config;
    config.initConfigStyle = "preset";
    config.presetStructure = "file";
    config.inputFile.clear();
    const grav_config::ScenarioValidationReport report =
        /// Description: Executes the evaluate operation.
        grav_config::SimulationScenarioValidation::evaluate(config);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(report.validForRun);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "input_file"));
}
/// Description: Executes the TEST operation.
TEST(ScenarioValidationTest, TST_UNT_CONF_059_DiskOrbitPresetChecksMassAndRadius)
{
    SimulationConfig config;
    config.initConfigStyle = "detailed";
    config.initMode = "disk_orbit";
    config.initDiskMass = 0.0f;
    config.initDiskRadiusMin = 4.0f;
    config.initDiskRadiusMax = 3.0f;
    const grav_config::ScenarioValidationReport report =
        /// Description: Executes the evaluate operation.
        grav_config::SimulationScenarioValidation::evaluate(config);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(report.validForRun);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "init_disk_mass"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "init_disk_radius"));
}
/// Description: Executes the TEST operation.
TEST(ScenarioValidationTest, TST_UNT_CONF_060_SphValidationChecksParametersAndLowParticleWarning)
{
    SimulationConfig config;
    config.sphEnabled = true;
    config.particleCount = 8u;
    config.sphSmoothingLength = 0.0f;
    config.sphRestDensity = 0.0f;
    config.sphGasConstant = 0.0f;
    config.sphViscosity = -1.0f;
    const grav_config::ScenarioValidationReport report =
        /// Description: Executes the evaluate operation.
        grav_config::SimulationScenarioValidation::evaluate(config);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(report.validForRun);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "sph"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "sph_viscosity"));
}
/// Description: Executes the TEST operation.
TEST(ScenarioValidationTest, TST_UNT_CONF_061_OctreeSolverValidationChecksCriterionAndThetaRange)
{
    SimulationConfig config;
    config.solver = "octree_gpu";
    config.octreeOpeningCriterion = "bad";
    config.octreeTheta = 0.0f;
    config.octreeThetaAutoMin = 0.0f;
    config.octreeThetaAutoMax = -1.0f;
    const grav_config::ScenarioValidationReport report =
        /// Description: Executes the evaluate operation.
        grav_config::SimulationScenarioValidation::evaluate(config);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(report.validForRun);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "octree_opening_criterion"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "octree_theta"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "octree_theta_auto_min"));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasField(report, "octree_theta_auto_max"));
}
/// Description: Executes the TEST operation.
TEST(ScenarioValidationTest, TST_UNT_CONF_062_RenderTextShowsBlockedSummaryAndActions)
{
    SimulationConfig config;
    config.particleCount = 1u;
    config.dt = 0.0f;
    const grav_config::ScenarioValidationReport report =
        /// Description: Executes the evaluate operation.
        grav_config::SimulationScenarioValidation::evaluate(config);
    const std::string rendered = grav_config::SimulationScenarioValidation::renderText(report);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(rendered.find("[preflight] blocked"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(rendered.find("error(s)"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(rendered.find("Action:"), std::string::npos);
}
} // namespace grav_test_config_scenario_validation
