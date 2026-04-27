// File: tests/unit/config/simulation_profile.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "config/SimulationConfig.hpp"
#include "config/SimulationProfile.hpp"
#include <gtest/gtest.h>
/// Description: Executes the TEST operation.
TEST(SimulationProfileTest, TST_UNT_CONF_046_NormalizeDiskOrbitProfile)
{
    std::string canonical;
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("disk_orbit", canonical));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(canonical, grav_config::kSimulationProfileDiskOrbit);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("DISK_ORBIT", canonical));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(canonical, grav_config::kSimulationProfileDiskOrbit);
}
/// Description: Executes the TEST operation.
TEST(SimulationProfileTest, TST_UNT_CONF_047_NormalizeProfilesAllValid)
{
    std::string canonical;
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("galaxy_collision", canonical));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(canonical, grav_config::kSimulationProfileGalaxyCollision);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("plummer_sphere", canonical));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(canonical, grav_config::kSimulationProfilePlummerSphere);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("binary_star", canonical));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(canonical, grav_config::kSimulationProfileBinaryStar);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("solar_system", canonical));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(canonical, grav_config::kSimulationProfileSolarSystem);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("sph_collapse", canonical));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(canonical, grav_config::kSimulationProfileSphCollapse);
}
/// Description: Executes the TEST operation.
TEST(SimulationProfileTest, TST_UNT_CONF_048_NormalizeRejectsInvalid)
{
    std::string canonical = "kept";
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_config::normalizeSimulationProfile("not_a_valid_profile", canonical));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(canonical, "kept");
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_config::normalizeSimulationProfile("", canonical));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_config::normalizeSimulationProfile("disk_orbit_typo", canonical));
}
/// Description: Executes the TEST operation.
TEST(SimulationProfileTest, TST_UNT_CONF_049_ApplySimulationProfileDiskOrbit)
{
    SimulationConfig config;
    config.simulationProfile = "disk_orbit";
    /// Description: Executes the applySimulationProfile operation.
    grav_config::applySimulationProfile(config);
    // Based on common settings for disk_orbit; we just verify something has been applied
    EXPECT_EQ(config.particleCount, 10000u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.solver, "pairwise_cuda");
}
/// Description: Executes the TEST operation.
TEST(SimulationProfileTest, TST_UNT_CONF_050_ApplySimulationProfileGalaxyCollision)
{
    SimulationConfig config;
    config.simulationProfile = "galaxy_collision";
    /// Description: Executes the applySimulationProfile operation.
    grav_config::applySimulationProfile(config);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.particleCount, 40000u);
}
/// Description: Executes the TEST operation.
TEST(SimulationProfileTest, TST_UNT_CONF_051_ApplySimulationProfilePlummerSphere)
{
    SimulationConfig config;
    config.simulationProfile = "PlUmMer_SpHERE"; // test weird-cased applies
    /// Description: Executes the applySimulationProfile operation.
    grav_config::applySimulationProfile(config);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.particleCount, 16384u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.solver, "octree_gpu");
}
/// Description: Executes the TEST operation.
TEST(SimulationProfileTest, TST_UNT_CONF_052_ApplySimulationProfileBinaryStar)
{
    SimulationConfig config;
    config.simulationProfile = "binary_star";
    /// Description: Executes the applySimulationProfile operation.
    grav_config::applySimulationProfile(config);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.particleCount, 2u);
}
/// Description: Executes the TEST operation.
TEST(SimulationProfileTest, TST_UNT_CONF_053_ApplySimulationProfileSolarSystem)
{
    SimulationConfig config;
    config.simulationProfile = "solar_system";
    /// Description: Executes the applySimulationProfile operation.
    grav_config::applySimulationProfile(config);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.particleCount, 10u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.solver, "pairwise_cuda");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.dt, 0.0001f);
}
/// Description: Executes the TEST operation.
TEST(SimulationProfileTest, TST_UNT_CONF_054_ApplySimulationProfileUnknownIsNoOp)
{
    SimulationConfig config;
    config.simulationProfile = "unknown_profile_123";
    config.particleCount = 42u;
    /// Description: Executes the applySimulationProfile operation.
    grav_config::applySimulationProfile(config);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.particleCount, 42u); // should not be touched
}
