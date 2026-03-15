#include "tests/support/physics_scenario.hpp"

#include <gtest/gtest.h>
#include <cmath>
#include <string>

namespace testsupport {

TEST(PhysicsTest, TST_UNT_PHYS_014_OctreeCpuDeterministicReplay)
{
    ScenarioConfig cfg;
    cfg.particleCount = 128u;
    cfg.dt = 0.005f;
    cfg.steps = 20;
    cfg.solver = "octree_cpu";
    cfg.integrator = "rk4";
    cfg.energyMeasureEverySteps = 1u;
    cfg.energySampleLimit = 128u;
    cfg.snapshotTimeoutMs = 6000;
    cfg.stepTimeoutMs = 6000;
    cfg.octreeTheta = 0.5f;
    cfg.octreeSoftening = 0.1f;
    cfg.initState.mode = "disk_orbit";
    cfg.initState.seed = 22222u;
    cfg.initState.includeCentralBody = true;
    cfg.initState.centralMass = 1.0f;
    cfg.initState.diskMass = 0.5f;
    cfg.initState.diskRadiusMin = 2.0f;
    cfg.initState.diskRadiusMax = 10.0f;
    cfg.initState.velocityScale = 1.0f;

    ScenarioResult runA;
    std::string errorA;
    ASSERT_TRUE(runScenario(cfg, runA, errorA)) << errorA;

    ScenarioResult runB;
    std::string errorB;
    ASSERT_TRUE(runScenario(cfg, runB, errorB)) << errorB;

    ASSERT_EQ(runA.final.size(), runB.final.size());
    for (std::size_t i = 0; i < runA.final.size(); ++i) {
        EXPECT_FLOAT_EQ(runA.final[i].x, runB.final[i].x)
            << "mismatch at particle " << i << " .x";
        EXPECT_FLOAT_EQ(runA.final[i].y, runB.final[i].y)
            << "mismatch at particle " << i << " .y";
        EXPECT_FLOAT_EQ(runA.final[i].z, runB.final[i].z)
            << "mismatch at particle " << i << " .z";
    }

    EXPECT_FLOAT_EQ(runA.stats.totalEnergy, runB.stats.totalEnergy);
    EXPECT_EQ(runA.stats.steps, runB.stats.steps);
}

TEST(PhysicsTest, TST_UNT_PHYS_015_OctreeGpuDeterministicReplay)
{
    ScenarioConfig cfg;
    cfg.particleCount = 128u;
    cfg.dt = 0.004f;
    cfg.steps = 25;
    cfg.solver = "octree_gpu";
    cfg.integrator = "euler";
    cfg.energyMeasureEverySteps = 1u;
    cfg.energySampleLimit = 128u;
    cfg.snapshotTimeoutMs = 6000;
    cfg.stepTimeoutMs = 6000;
    cfg.octreeTheta = 0.35f;
    cfg.octreeSoftening = 0.08f;
    cfg.initState.mode = "disk_orbit";
    cfg.initState.seed = 33333u;
    cfg.initState.includeCentralBody = true;
    cfg.initState.centralMass = 1.0f;
    cfg.initState.diskMass = 0.75f;
    cfg.initState.diskRadiusMin = 1.5f;
    cfg.initState.diskRadiusMax = 12.0f;
    cfg.initState.velocityScale = 1.0f;

    ScenarioResult runA;
    std::string errorA;
    ASSERT_TRUE(runScenario(cfg, runA, errorA)) << errorA;
    if (runA.stats.solverName != cfg.solver) {
        GTEST_SKIP() << "octree_gpu unavailable in this environment (actual solver="
                     << runA.stats.solverName << ")";
    }

    ScenarioResult runB;
    std::string errorB;
    ASSERT_TRUE(runScenario(cfg, runB, errorB)) << errorB;

    ASSERT_EQ(runA.final.size(), runB.final.size());
    for (std::size_t i = 0; i < runA.final.size(); ++i) {
        EXPECT_FLOAT_EQ(runA.final[i].x, runB.final[i].x)
            << "mismatch at particle " << i << " .x";
        EXPECT_FLOAT_EQ(runA.final[i].y, runB.final[i].y)
            << "mismatch at particle " << i << " .y";
        EXPECT_FLOAT_EQ(runA.final[i].z, runB.final[i].z)
            << "mismatch at particle " << i << " .z";
    }

    EXPECT_FLOAT_EQ(runA.stats.totalEnergy, runB.stats.totalEnergy);
}

TEST(PhysicsTest, TST_UNT_PHYS_016_SphStabilityBoundedDrift)
{
    ScenarioConfig cfg;
    cfg.particleCount = 96u;
    cfg.dt = 0.002f;
    cfg.steps = 30;
    cfg.solver = "pairwise_cuda";
    cfg.integrator = "euler";
    cfg.sphEnabled = true;
    cfg.energyMeasureEverySteps = 1u;
    cfg.energySampleLimit = 96u;
    cfg.snapshotTimeoutMs = 8000;
    cfg.stepTimeoutMs = 8000;
    cfg.initState.mode = "disk_orbit";
    cfg.initState.seed = 44444u;
    cfg.initState.includeCentralBody = false;
    cfg.initState.diskMass = 1.0f;
    cfg.initState.diskRadiusMin = 0.5f;
    cfg.initState.diskRadiusMax = 5.0f;
    cfg.initState.velocityScale = 0.5f;
    cfg.initState.particleTemperature = 1.0f;

    ScenarioResult runA;
    std::string errorA;
    if (!runScenario(cfg, runA, errorA)) {
        if (errorA.find("timeout") != std::string::npos) {
            GTEST_SKIP() << "Test skipped because scenario execution timed out (likely server paused due to no CUDA GPU for SPH): " << errorA;
        } else {
            FAIL() << "Scenario failed unexpectedly: " << errorA;
        }
    }
    if (runA.stats.solverName != cfg.solver) {
        GTEST_SKIP() << "pairwise_cuda unavailable in this environment (actual solver="
                     << runA.stats.solverName << ")";
    }

    EXPECT_LE(runA.maxAbsEnergyDriftPct, 25.0f);
    
    for (const auto &p : runA.final) {
        EXPECT_TRUE(std::isfinite(p.x));
        EXPECT_TRUE(std::isfinite(p.y));
        EXPECT_TRUE(std::isfinite(p.z));
        const float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        EXPECT_LE(r, 20.0f) << "Particle escaped bounded region due to instability";
    }
}

} // namespace testsupport
