#include "config/SimulationScenarioValidation.hpp"

#include "config/SimulationConfig.hpp"

#include <gtest/gtest.h>

#include <string>

namespace grav_test_config_scenario_validation_init_modes {

static bool hasField(const grav_config::ScenarioValidationReport &report, const std::string &field)
{
    for (const grav_config::ScenarioDiagnostic &diagnostic : report.diagnostics) {
        if (diagnostic.field == field) {
            return true;
        }
    }
    return false;
}

TEST(ScenarioValidationInitModesTest, TST_UNT_CONF_063_DetailedRandomCloudValidatesGeneratedMassAndExtent)
{
    SimulationConfig config;
    config.initConfigStyle = "detailed";
    config.initMode = "random_cloud";
    config.initIncludeCentralBody = false;
    config.initParticleMass = 0.0f;
    config.initCloudHalfExtent = 0.0f;

    const grav_config::ScenarioValidationReport report = grav_config::SimulationScenarioValidation::evaluate(config);

    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "init_particle_mass"));
    EXPECT_TRUE(hasField(report, "init_cloud_half_extent"));
}

TEST(ScenarioValidationInitModesTest, TST_UNT_CONF_064_DetailedDiskOrbitCentralBodyRequiresPositiveMass)
{
    SimulationConfig config;
    config.initConfigStyle = "detailed";
    config.initMode = "disk_orbit";
    config.initIncludeCentralBody = true;
    config.initCentralMass = 0.0f;
    config.initParticleMass = 1.0f;
    config.initDiskMass = 1.0f;
    config.initDiskRadiusMin = 1.0f;
    config.initDiskRadiusMax = 2.0f;

    const grav_config::ScenarioValidationReport report = grav_config::SimulationScenarioValidation::evaluate(config);

    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "init_central_mass"));
}

TEST(ScenarioValidationInitModesTest, TST_UNT_CONF_065_DetailedFileModeWithoutInputFileIsRejected)
{
    SimulationConfig config;
    config.initConfigStyle = "detailed";
    config.initMode = "file";
    config.inputFile.clear();

    const grav_config::ScenarioValidationReport report = grav_config::SimulationScenarioValidation::evaluate(config);

    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "input_file"));
}

TEST(ScenarioValidationInitModesTest, TST_UNT_CONF_066_OctreeAutoMaxLowerThanMinRaisesError)
{
    SimulationConfig config;
    config.solver = "octree_cpu";
    config.octreeOpeningCriterion = "com";
    config.octreeTheta = 1.0f;
    config.octreeThetaAutoMin = 0.8f;
    config.octreeThetaAutoMax = 0.2f;

    const grav_config::ScenarioValidationReport report = grav_config::SimulationScenarioValidation::evaluate(config);

    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "octree_theta_auto_max"));
}

TEST(ScenarioValidationInitModesTest, TST_UNT_CONF_067_PhysicsMinDistanceAboveSofteningSquareTriggersWarning)
{
    SimulationConfig config;
    config.octreeSoftening = 0.5f;
    config.physicsMinDistance2 = 0.4f;

    const grav_config::ScenarioValidationReport report = grav_config::SimulationScenarioValidation::evaluate(config);

    EXPECT_TRUE(report.validForRun);
    EXPECT_TRUE(hasField(report, "physics_min_distance2"));
}

} // namespace grav_test_config_scenario_validation_init_modes
