// File: tests/unit/physics/TST_PHY_ProfileTests.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "config/SimulationConfig.hpp"
#include "config/SimulationProfile.hpp"
#include <gtest/gtest.h>
namespace grav_test {
/// Description: Executes the TEST operation.
TEST(SimulationProfileTests, TST_PHY_PRO_001_DiskOrbitProfile)
{
    SimulationConfig config;
    config.simulationProfile = "disk_orbit";
    /// Description: Executes the applySimulationProfile operation.
    grav_config::applySimulationProfile(config);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.simulationProfile, "disk_orbit");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.particleCount, 10000u);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(config.dt, 0.01f, 1e-6f);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.solver, "pairwise_cuda");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.initMode, "disk_orbit");
}
/// Description: Executes the TEST operation.
TEST(SimulationProfileTests, TST_PHY_PRO_002_GalaxyCollisionProfile)
{
    SimulationConfig config;
    config.simulationProfile = "galaxy_collision";
    /// Description: Executes the applySimulationProfile operation.
    grav_config::applySimulationProfile(config);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.simulationProfile, "galaxy_collision");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.particleCount, 40000u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.solver, "octree_gpu");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.initMode, "galaxy_collision");
}
/// Description: Executes the TEST operation.
TEST(SimulationProfileTests, TST_PHY_PRO_003_SolarSystemProfile)
{
    SimulationConfig config;
    config.simulationProfile = "solar_system";
    /// Description: Executes the applySimulationProfile operation.
    grav_config::applySimulationProfile(config);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.simulationProfile, "solar_system");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.particleCount, 10u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.integrator, "rk4");
    /// Description: Executes the EXPECT_LT operation.
    EXPECT_LT(config.dt, 0.001f);
}
/// Description: Executes the TEST operation.
TEST(SimulationProfileTests, TST_PHY_PRO_004_InvalidProfile)
{
    SimulationConfig config;
    config.particleCount = 123u;
    config.simulationProfile = "invalid_profile_name";
    /// Description: Executes the applySimulationProfile operation.
    grav_config::applySimulationProfile(config);
    // Should not change existing config if profile is invalid
    EXPECT_EQ(config.simulationProfile, "invalid_profile_name");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.particleCount, 123u);
}
/// Description: Executes the TEST operation.
TEST(SimulationProfileTests, TST_PHY_PRO_005_BinaryStarProfile)
{
    SimulationConfig config;
    config.simulationProfile = "binary_star";
    /// Description: Executes the applySimulationProfile operation.
    grav_config::applySimulationProfile(config);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.simulationProfile, "binary_star");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.particleCount, 2u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.integrator, "rk4");
}
} // namespace grav_test
