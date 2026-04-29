/*
 * @file tests/unit/config/scenario_validation_init_modes.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "config/SimulationConfig.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include <gtest/gtest.h>
#include <string>

namespace bltzr_test_config_scenario_validation_init_modes {
static bool hasField(const bltzr_config::ScenarioValidationReport& report, const std::string& field)
{
    for (const bltzr_config::ScenarioDiagnostic& diagnostic : report.diagnostics)
        if (diagnostic.field == field)
            return true;
    return false;
}

TEST(ScenarioValidationInitModesTest,
     TST_UNT_CONF_063_DetailedRandomCloudValidatesGeneratedMassAndExtent)
{
    SimulationConfig config;
    config.initConfigStyle = "detailed";
    config.initMode = "random_cloud";
    config.initIncludeCentralBody = false;
    config.initParticleMass = 0.0f;
    config.initCloudHalfExtent = 0.0f;
    const bltzr_config::ScenarioValidationReport report =
        bltzr_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "init_particle_mass"));
    EXPECT_TRUE(hasField(report, "init_cloud_half_extent"));
}

TEST(ScenarioValidationInitModesTest,
     TST_UNT_CONF_068_DetailedCubeRandomValidatesGeneratedMassAndExtent)
{
    SimulationConfig config;
    config.initConfigStyle = "detailed";
    config.initMode = "cube_random";
    config.initIncludeCentralBody = false;
    config.initParticleMass = 0.0f;
    config.initCubeHalfExtent = 0.0f;
    const bltzr_config::ScenarioValidationReport report =
        bltzr_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "init_particle_mass"));
    EXPECT_TRUE(hasField(report, "init_cube_half_extent"));
}

TEST(ScenarioValidationInitModesTest,
     TST_UNT_CONF_069_DetailedSphereRandomValidatesGeneratedMassAndRadius)
{
    SimulationConfig config;
    config.initConfigStyle = "detailed";
    config.initMode = "sphere_random";
    config.initIncludeCentralBody = false;
    config.initParticleMass = 0.0f;
    config.initSphereRadius = 0.0f;
    const bltzr_config::ScenarioValidationReport report =
        bltzr_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "init_particle_mass"));
    EXPECT_TRUE(hasField(report, "init_sphere_radius"));
}

TEST(ScenarioValidationInitModesTest,
     TST_UNT_CONF_064_DetailedDiskOrbitCentralBodyRequiresPositiveMass)
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
    const bltzr_config::ScenarioValidationReport report =
        bltzr_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "init_central_mass"));
}

TEST(ScenarioValidationInitModesTest, TST_UNT_CONF_065_DetailedFileModeWithoutInputFileIsRejected)
{
    SimulationConfig config;
    config.initConfigStyle = "detailed";
    config.initMode = "file";
    config.inputFile.clear();
    const bltzr_config::ScenarioValidationReport report =
        bltzr_config::SimulationScenarioValidation::evaluate(config);
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
    const bltzr_config::ScenarioValidationReport report =
        bltzr_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_FALSE(report.validForRun);
    EXPECT_TRUE(hasField(report, "octree_theta_auto_max"));
}

TEST(ScenarioValidationInitModesTest,
     TST_UNT_CONF_067_PhysicsMinDistanceAboveSofteningSquareTriggersWarning)
{
    SimulationConfig config;
    config.octreeSoftening = 0.5f;
    config.physicsMinDistance2 = 0.4f;
    const bltzr_config::ScenarioValidationReport report =
        bltzr_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_TRUE(report.validForRun);
    EXPECT_TRUE(hasField(report, "physics_min_distance2"));
}
} // namespace bltzr_test_config_scenario_validation_init_modes
