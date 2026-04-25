#include "config/SimulationConfig.hpp"
#include "server/SimulationInitConfig.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <string>
TEST(ConfigInitPlanEdgesTest, TST_UNT_CONF_084_PresetFileModeKeepsConfiguredInputAndAutoFormat)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.initConfigStyle = "preset";
    config.presetStructure = "file";
    config.inputFile = "tests/data/two_body_rest.xyz";
    config.inputFormat.clear();
    std::stringstream log;
    const ResolvedInitialStatePlan plan = resolveInitialStatePlan(config, log);
    EXPECT_EQ(plan.config.mode, "file");
    EXPECT_EQ(plan.inputFile, "tests/data/two_body_rest.xyz");
    EXPECT_EQ(plan.inputFormat, "auto");
    EXPECT_NE(plan.summary.find("source=file"), std::string::npos);
}
TEST(ConfigInitPlanEdgesTest, TST_UNT_CONF_085_PresetFileModeKeepsExplicitInputFormat)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.initConfigStyle = "preset";
    config.presetStructure = "file";
    config.inputFile = "tests/data/two_body_rest.xyz";
    config.inputFormat = "xyz";
    std::stringstream log;
    const ResolvedInitialStatePlan plan = resolveInitialStatePlan(config, log);
    EXPECT_EQ(plan.config.mode, "file");
    EXPECT_EQ(plan.inputFormat, "xyz");
    EXPECT_NE(plan.summary.find("input_format=xyz"), std::string::npos);
}
TEST(ConfigInitPlanEdgesTest, TST_UNT_CONF_086_PresetRandomCloudClampsMassForSingleParticle)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.initConfigStyle = "preset";
    config.presetStructure = "random_cloud";
    config.particleCount = 1u;
    config.presetSize = 4.0f;
    std::stringstream log;
    const ResolvedInitialStatePlan plan = resolveInitialStatePlan(config, log);
    EXPECT_EQ(plan.config.mode, "random_cloud");
    EXPECT_FALSE(plan.config.includeCentralBody);
    EXPECT_FLOAT_EQ(plan.config.cloudHalfExtent, 4.0f);
    EXPECT_FLOAT_EQ(plan.config.particleMass, 0.5f);
}
TEST(ConfigInitPlanEdgesTest, TST_UNT_CONF_087_PresetTwoBodyResolvesGeneratedOrbitDefaults)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.initConfigStyle = "preset";
    config.presetStructure = "two_body";
    config.presetSize = 6.0f;
    std::stringstream log;
    const ResolvedInitialStatePlan plan = resolveInitialStatePlan(config, log);
    EXPECT_EQ(plan.config.mode, "two_body");
    EXPECT_FALSE(plan.config.includeCentralBody);
    EXPECT_FLOAT_EQ(plan.config.centralMass, 0.0f);
    EXPECT_FLOAT_EQ(plan.config.cloudHalfExtent, 6.0f);
    EXPECT_FLOAT_EQ(plan.config.velocityScale, 1.0f);
    EXPECT_FLOAT_EQ(plan.config.particleMass, 1.0f);
    EXPECT_NE(plan.summary.find("source=generated"), std::string::npos);
}
TEST(ConfigInitPlanEdgesTest, TST_UNT_CONF_088_PresetPlummerSphereComputesParticleMassFromCount)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.initConfigStyle = "preset";
    config.presetStructure = "plummer_sphere";
    config.particleCount = 10u;
    config.presetSize = 9.0f;
    std::stringstream log;
    const ResolvedInitialStatePlan plan = resolveInitialStatePlan(config, log);
    EXPECT_EQ(plan.config.mode, "plummer_sphere");
    EXPECT_FALSE(plan.config.includeCentralBody);
    EXPECT_FLOAT_EQ(plan.config.centralMass, 0.0f);
    EXPECT_FLOAT_EQ(plan.config.cloudHalfExtent, 9.0f);
    EXPECT_FLOAT_EQ(plan.config.velocityScale, 1.0f);
    EXPECT_FLOAT_EQ(plan.config.particleMass, 0.1f);
}
