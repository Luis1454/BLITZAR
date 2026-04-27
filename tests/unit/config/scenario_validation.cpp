/*
 * @file tests/unit/config/scenario_validation.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "config/SimulationConfig.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include <gtest/gtest.h>
#include <string>

namespace grav_test_config_scenario_validation {
static bool hasField(const grav_config::ScenarioValidationReport& report, const std::string& field)
{
    for (const grav_config::ScenarioDiagnostic& diagnostic : report.diagnostics)
        if (diagnostic.field == field)
            return true;
    return false;
}

TEST(ScenarioValidationTest, TST_UNT_CONF_055_DefaultConfigurationIsRunnable)
{
    const SimulationConfig config;
    const grav_config::ScenarioValidationReport report =
        grav_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_TRUE(report.validForRun);
    EXPECT_EQ(report.errorCount, 0u);
    const std::string rendered = grav_config::SimulationScenarioValidation::renderText(report);
    EXPECT_NE(rendered.find("[preflight] OK"), std::string::npos);
}

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
        grav_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_FALSE(report.validForRun);
    EXPECT_GT(report.errorCount, 0u);
    EXPECT_TRUE(hasField(report, "particle_count"));
    EXPECT_TRUE(hasField(report, "dt"));
    EXPECT_TRUE(hasField(report, "max_substeps"));
    EXPECT_TRUE(hasField(report, "client_snapshot_queue_capacity"));
    EXPECT_TRUE(hasField(report, "client_snapshot_drop_policy"));
    EXPECT_TRUE(hasField(report, "ui_theme"));
    EXPECT_TRUE(hasField(report, "octree_softening"));
}

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
        grav_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_TRUE(report.validForRun);
    EXPECT_EQ(report.errorCount, 0u);
    EXPECT_GT(report.warningCount, 0u);
    EXPECT_TRUE(hasField(report, "particle_count"));
    EXPECT_TRUE(hasField(report, "dt"));
    EXPECT_TRUE(hasField(report, "substep_target_dt"));
    EXPECT_TRUE(hasField(report, "client_snapshot_queue_capacity"));
    EXPECT_TRUE(hasField(report, "octree_softening"));
}

TEST(ScenarioValidationTest, TST_UNT_CONF_058_FileModeWithoutInputProducesError)
{
    SimulationConfig config;
    config.initConfigStyle = "preset";
    config.presetStructure = "file";
    config.inputFile.clear();
    const grav_config::ScenarioValidationReport report =
        grav_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "input_file"));
}

TEST(ScenarioValidationTest, TST_UNT_CONF_059_DiskOrbitPresetChecksMassAndRadius)
{
    SimulationConfig config;
    config.initConfigStyle = "detailed";
    config.initMode = "disk_orbit";
    config.initDiskMass = 0.0f;
    config.initDiskRadiusMin = 4.0f;
    config.initDiskRadiusMax = 3.0f;
    const grav_config::ScenarioValidationReport report =
        grav_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "init_disk_mass"));
    EXPECT_TRUE(hasField(report, "init_disk_radius"));
}

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
        grav_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "sph"));
    EXPECT_TRUE(hasField(report, "sph_viscosity"));
}

TEST(ScenarioValidationTest, TST_UNT_CONF_061_OctreeSolverValidationChecksCriterionAndThetaRange)
{
    SimulationConfig config;
    config.solver = "octree_gpu";
    config.octreeOpeningCriterion = "bad";
    config.octreeTheta = 0.0f;
    config.octreeThetaAutoMin = 0.0f;
    config.octreeThetaAutoMax = -1.0f;
    const grav_config::ScenarioValidationReport report =
        grav_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "octree_opening_criterion"));
    EXPECT_TRUE(hasField(report, "octree_theta"));
    EXPECT_TRUE(hasField(report, "octree_theta_auto_min"));
    EXPECT_TRUE(hasField(report, "octree_theta_auto_max"));
}

TEST(ScenarioValidationTest, TST_UNT_CONF_062_RenderTextShowsBlockedSummaryAndActions)
{
    SimulationConfig config;
    config.particleCount = 1u;
    config.dt = 0.0f;
    const grav_config::ScenarioValidationReport report =
        grav_config::SimulationScenarioValidation::evaluate(config);
    const std::string rendered = grav_config::SimulationScenarioValidation::renderText(report);
    EXPECT_NE(rendered.find("[preflight] blocked"), std::string::npos);
    EXPECT_NE(rendered.find("error(s)"), std::string::npos);
    EXPECT_NE(rendered.find("Action:"), std::string::npos);
}
} // namespace grav_test_config_scenario_validation
