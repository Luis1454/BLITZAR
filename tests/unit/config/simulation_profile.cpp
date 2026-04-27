// File: tests/unit/config/simulation_profile.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "config/SimulationConfig.hpp"
#include "config/SimulationProfile.hpp"
#include <gtest/gtest.h>
TEST(SimulationProfileTest, TST_UNT_CONF_046_NormalizeDiskOrbitProfile)
{
    std::string canonical;
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("disk_orbit", canonical));
    EXPECT_EQ(canonical, grav_config::kSimulationProfileDiskOrbit);
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("DISK_ORBIT", canonical));
    EXPECT_EQ(canonical, grav_config::kSimulationProfileDiskOrbit);
}
TEST(SimulationProfileTest, TST_UNT_CONF_047_NormalizeProfilesAllValid)
{
    std::string canonical;
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("galaxy_collision", canonical));
    EXPECT_EQ(canonical, grav_config::kSimulationProfileGalaxyCollision);
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("plummer_sphere", canonical));
    EXPECT_EQ(canonical, grav_config::kSimulationProfilePlummerSphere);
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("binary_star", canonical));
    EXPECT_EQ(canonical, grav_config::kSimulationProfileBinaryStar);
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("solar_system", canonical));
    EXPECT_EQ(canonical, grav_config::kSimulationProfileSolarSystem);
    EXPECT_TRUE(grav_config::normalizeSimulationProfile("sph_collapse", canonical));
    EXPECT_EQ(canonical, grav_config::kSimulationProfileSphCollapse);
}
TEST(SimulationProfileTest, TST_UNT_CONF_048_NormalizeRejectsInvalid)
{
    std::string canonical = "kept";
    EXPECT_FALSE(grav_config::normalizeSimulationProfile("not_a_valid_profile", canonical));
    EXPECT_EQ(canonical, "kept");
    EXPECT_FALSE(grav_config::normalizeSimulationProfile("", canonical));
    EXPECT_FALSE(grav_config::normalizeSimulationProfile("disk_orbit_typo", canonical));
}
TEST(SimulationProfileTest, TST_UNT_CONF_049_ApplySimulationProfileDiskOrbit)
{
    SimulationConfig config;
    config.simulationProfile = "disk_orbit";
    grav_config::applySimulationProfile(config);
    // Based on common settings for disk_orbit; we just verify something has been applied
    EXPECT_EQ(config.particleCount, 10000u);
    EXPECT_EQ(config.solver, "pairwise_cuda");
}
TEST(SimulationProfileTest, TST_UNT_CONF_050_ApplySimulationProfileGalaxyCollision)
{
    SimulationConfig config;
    config.simulationProfile = "galaxy_collision";
    grav_config::applySimulationProfile(config);
    EXPECT_EQ(config.particleCount, 40000u);
}
TEST(SimulationProfileTest, TST_UNT_CONF_051_ApplySimulationProfilePlummerSphere)
{
    SimulationConfig config;
    config.simulationProfile = "PlUmMer_SpHERE"; // test weird-cased applies
    grav_config::applySimulationProfile(config);
    EXPECT_EQ(config.particleCount, 16384u);
    EXPECT_EQ(config.solver, "octree_gpu");
}
TEST(SimulationProfileTest, TST_UNT_CONF_052_ApplySimulationProfileBinaryStar)
{
    SimulationConfig config;
    config.simulationProfile = "binary_star";
    grav_config::applySimulationProfile(config);
    EXPECT_EQ(config.particleCount, 2u);
}
TEST(SimulationProfileTest, TST_UNT_CONF_053_ApplySimulationProfileSolarSystem)
{
    SimulationConfig config;
    config.simulationProfile = "solar_system";
    grav_config::applySimulationProfile(config);
    EXPECT_EQ(config.particleCount, 10u);
    EXPECT_EQ(config.solver, "pairwise_cuda");
    EXPECT_EQ(config.dt, 0.0001f);
}
TEST(SimulationProfileTest, TST_UNT_CONF_054_ApplySimulationProfileUnknownIsNoOp)
{
    SimulationConfig config;
    config.simulationProfile = "unknown_profile_123";
    config.particleCount = 42u;
    grav_config::applySimulationProfile(config);
    EXPECT_EQ(config.particleCount, 42u); // should not be touched
}
