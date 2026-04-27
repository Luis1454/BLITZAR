// File: tests/unit/physics/TST_PHY_ProfileTests.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "config/SimulationConfig.hpp"
#include "config/SimulationProfile.hpp"
#include <gtest/gtest.h>
namespace grav_test {
TEST(SimulationProfileTests, TST_PHY_PRO_001_DiskOrbitProfile)
{
    SimulationConfig config;
    config.simulationProfile = "disk_orbit";
    grav_config::applySimulationProfile(config);
    EXPECT_EQ(config.simulationProfile, "disk_orbit");
    EXPECT_EQ(config.particleCount, 10000u);
    EXPECT_NEAR(config.dt, 0.01f, 1e-6f);
    EXPECT_EQ(config.solver, "pairwise_cuda");
    EXPECT_EQ(config.initMode, "disk_orbit");
}
TEST(SimulationProfileTests, TST_PHY_PRO_002_GalaxyCollisionProfile)
{
    SimulationConfig config;
    config.simulationProfile = "galaxy_collision";
    grav_config::applySimulationProfile(config);
    EXPECT_EQ(config.simulationProfile, "galaxy_collision");
    EXPECT_EQ(config.particleCount, 40000u);
    EXPECT_EQ(config.solver, "octree_gpu");
    EXPECT_EQ(config.initMode, "galaxy_collision");
}
TEST(SimulationProfileTests, TST_PHY_PRO_003_SolarSystemProfile)
{
    SimulationConfig config;
    config.simulationProfile = "solar_system";
    grav_config::applySimulationProfile(config);
    EXPECT_EQ(config.simulationProfile, "solar_system");
    EXPECT_EQ(config.particleCount, 10u);
    EXPECT_EQ(config.integrator, "rk4");
    EXPECT_LT(config.dt, 0.001f);
}
TEST(SimulationProfileTests, TST_PHY_PRO_004_InvalidProfile)
{
    SimulationConfig config;
    config.particleCount = 123u;
    config.simulationProfile = "invalid_profile_name";
    grav_config::applySimulationProfile(config);
    // Should not change existing config if profile is invalid
    EXPECT_EQ(config.simulationProfile, "invalid_profile_name");
    EXPECT_EQ(config.particleCount, 123u);
}
TEST(SimulationProfileTests, TST_PHY_PRO_005_BinaryStarProfile)
{
    SimulationConfig config;
    config.simulationProfile = "binary_star";
    grav_config::applySimulationProfile(config);
    EXPECT_EQ(config.simulationProfile, "binary_star");
    EXPECT_EQ(config.particleCount, 2u);
    EXPECT_EQ(config.integrator, "rk4");
}
} // namespace grav_test
