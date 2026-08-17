/*
 * @file engine/physics/octree/tests/extended_solvers.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

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

} // namespace testsupport
