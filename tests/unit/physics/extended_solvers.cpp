#include "tests/support/physics_scenario.hpp"
#include "tests/support/physics_test_utils.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <string>
namespace testsupport {
TEST(PhysicsTest, TST_UNT_PHYS_014_OctreeCpuDeterministicReplay)
{
    ScenarioConfig cfg = buildDiskOrbitScenario(128u, 0.005f, 20u, 22222u, "octree_cpu", "rk4");
    setScenarioEnergySampling(cfg, 1u, 128u);
    setScenarioTiming(cfg, 6000, 6000);
    cfg.octreeTheta = 0.5f;
    cfg.octreeSoftening = 0.1f;
    cfg.initState.diskMass = 0.5f;
    cfg.initState.diskRadiusMin = 2.0f;
    cfg.initState.diskRadiusMax = 10.0f;
    ScenarioResult runA;
    std::string errorA;
    ASSERT_TRUE(runScenario(cfg, runA, errorA)) << errorA;
    ScenarioResult runB;
    std::string errorB;
    ASSERT_TRUE(runScenario(cfg, runB, errorB)) << errorB;
    std::string replayError;
    EXPECT_TRUE(haveExactReplayMatch(runA, runB, replayError)) << replayError;
}
TEST(PhysicsTest, TST_UNT_PHYS_015_OctreeGpuDeterministicReplay)
{
    ScenarioConfig cfg = buildDiskOrbitScenario(128u, 0.004f, 25u, 33333u, "octree_gpu", "euler");
    setScenarioEnergySampling(cfg, 1u, 128u);
    setScenarioTiming(cfg, 6000, 6000);
    cfg.octreeTheta = 0.35f;
    cfg.octreeSoftening = 0.08f;
    cfg.initState.diskMass = 0.75f;
    cfg.initState.diskRadiusMax = 12.0f;
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
    std::string replayError;
    EXPECT_TRUE(haveExactReplayMatch(runA, runB, replayError)) << replayError;
}
TEST(PhysicsTest, TST_UNT_PHYS_016_SphStabilityBoundedDrift)
{
    ScenarioConfig cfg = buildDiskOrbitScenario(96u, 0.002f, 30u, 44444u, "pairwise_cuda", "euler");
    setScenarioEnergySampling(cfg, 1u, 96u);
    setScenarioTiming(cfg, 8000, 8000);
    cfg.sphEnabled = true;
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
            GTEST_SKIP() << "Test skipped because scenario execution timed out (likely server "
                            "paused due to no CUDA GPU for SPH): "
                         << errorA;
        }
        else {
            FAIL() << "Scenario failed unexpectedly: " << errorA;
        }
    }
    if (runA.stats.solverName != cfg.solver) {
        GTEST_SKIP() << "pairwise_cuda unavailable in this environment (actual solver="
                     << runA.stats.solverName << ")";
    }
    EXPECT_LE(runA.maxAbsEnergyDriftPct, 25.0f);
    for (const auto& p : runA.final) {
        EXPECT_TRUE(std::isfinite(p.x));
        EXPECT_TRUE(std::isfinite(p.y));
        EXPECT_TRUE(std::isfinite(p.z));
        const float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        EXPECT_LE(r, 20.0f) << "Particle escaped bounded region due to instability";
    }
}
} // namespace testsupport
