// File: tests/unit/physics/orbit.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "tests/support/physics_scenario.hpp"
#include "tests/support/physics_test_utils.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <gtest/gtest.h>
#include <string>
#include <vector>
namespace testsupport {
TEST(PhysicsTest, TST_UNT_PHYS_001_AttractionDistance)
{
    ScenarioConfig cfg;
    std::string error;
    ASSERT_TRUE(buildTwoBodyFileScenario(cfg, 400u, 0.002f, "octree_cpu", "rk4", error)) << error;
    ScenarioResult result;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    const float initialDistance = distance(result.initial[0], result.initial[1]);
    const float finalDistance = distance(result.final[0], result.final[1]);
    const float ratio = finalDistance / std::max(initialDistance, 1e-8f);
    constexpr float kMinRatio = 0.05f;
    constexpr float kMaxRatio = 0.999f;
    EXPECT_GE(ratio, kMinRatio) << "Distance collapsed too much";
    EXPECT_LE(ratio, kMaxRatio) << "No attraction detected";
}
TEST(PhysicsTest, TST_UNT_PHYS_003_CenterOfMassDrift)
{
    ScenarioConfig cfg;
    std::string error;
    ASSERT_TRUE(buildTwoBodyFileScenario(cfg, 400u, 0.002f, "octree_cpu", "rk4", error)) << error;
    ScenarioResult result;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    const auto centerOfMass = [](const std::vector<RenderParticle>& snapshot) {
        const float totalMass = snapshot[0].mass + snapshot[1].mass;
        const float cx =
            (snapshot[0].x * snapshot[0].mass + snapshot[1].x * snapshot[1].mass) / totalMass;
        const float cy =
            (snapshot[0].y * snapshot[0].mass + snapshot[1].y * snapshot[1].mass) / totalMass;
        const float cz =
            (snapshot[0].z * snapshot[0].mass + snapshot[1].z * snapshot[1].mass) / totalMass;
        return std::array<float, 3>{cx, cy, cz};
    };
    const auto c0 = centerOfMass(result.initial);
    const auto c1 = centerOfMass(result.final);
    const float drift =
        std::sqrt((c1[0] - c0[0]) * (c1[0] - c0[0]) + (c1[1] - c0[1]) * (c1[1] - c0[1]) +
                  (c1[2] - c0[2]) * (c1[2] - c0[2]));
    constexpr float kMaxCenterOfMassDrift = 1e-2f;
    EXPECT_LE(drift, kMaxCenterOfMassDrift);
}
TEST(PhysicsTest, TST_UNT_PHYS_004_TimeStepConvergence)
{
    ScenarioConfig coarse;
    ScenarioConfig fine;
    std::string coarseError;
    ASSERT_TRUE(buildTwoBodyFileScenario(coarse, 100u, 0.002f, "octree_cpu", "rk4", coarseError))
        << coarseError;
    std::string fineError;
    ASSERT_TRUE(buildTwoBodyFileScenario(fine, 200u, 0.001f, "octree_cpu", "rk4", fineError))
        << fineError;
    ScenarioResult coarseResult;
    ASSERT_TRUE(runScenario(coarse, coarseResult, coarseError)) << coarseError;
    ScenarioResult fineResult;
    ASSERT_TRUE(runScenario(fine, fineResult, fineError)) << fineError;
    float maxParticleDelta = 0.0f;
    for (std::size_t i = 0; i < 2; ++i) {
        maxParticleDelta =
            std::max(maxParticleDelta, distance(coarseResult.final[i], fineResult.final[i]));
    }
    constexpr float kMaxConvergenceDelta = 0.05f;
    EXPECT_LE(maxParticleDelta, kMaxConvergenceDelta);
}
TEST(PhysicsTest, TST_UNT_PHYS_002_EnergyConservation)
{
    ScenarioConfig cfg;
    std::string error;
    ASSERT_TRUE(buildTwoBodyFileScenario(cfg, 800u, 0.002f, "octree_cpu", "rk4", error)) << error;
    ScenarioResult result;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    constexpr float kMaxEnergyDriftPct = 5.0f;
    EXPECT_LE(result.maxAbsEnergyDriftPct, kMaxEnergyDriftPct)
        << "Energy drift too high: " << result.maxAbsEnergyDriftPct << "%";
    EXPECT_FALSE(result.stats.energyEstimated);
}
TEST(PhysicsTest, TST_UNT_PHYS_005_LongRunStability)
{
    ScenarioConfig cfg;
    std::string error;
    ASSERT_TRUE(buildTwoBodyFileScenario(cfg, 2000u, 0.001f, "octree_cpu", "rk4", error)) << error;
    ScenarioResult result;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    ASSERT_FALSE(result.final.empty());
    for (const RenderParticle& p : result.final) {
        EXPECT_TRUE(std::isfinite(p.x));
        EXPECT_TRUE(std::isfinite(p.y));
        EXPECT_TRUE(std::isfinite(p.z));
    }
    float maxRadius = 0.0f;
    for (const RenderParticle& p : result.final) {
        const float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        maxRadius = std::max(maxRadius, r);
    }
    constexpr float kMaxStableRadius = 50.0f;
    EXPECT_LE(maxRadius, kMaxStableRadius) << "Trajectory escaped expected stable bounds";
}
TEST(PhysicsTest, TST_UNT_PHYS_010_CalibrationTwoBodyPresetMaintainsBoundOrbit)
{
    ScenarioConfig cfg;
    std::string error;
    ASSERT_TRUE(prepareGeneratedCalibrationScenario("two_body", cfg, error)) << error;
    cfg.solver = "octree_cpu";
    ScenarioResult result;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    ASSERT_EQ(result.initial.size(), 2u);
    ASSERT_EQ(result.final.size(), 2u);
    const float initialDistance = distance(result.initial[0], result.initial[1]);
    const float finalDistance = distance(result.final[0], result.final[1]);
    const float orbitRatio = finalDistance / std::max(initialDistance, 1e-6f);
    const auto finalCenter = centerOfMassAll(result.final);
    const float centerMagnitude =
        std::sqrt(finalCenter[0] * finalCenter[0] + finalCenter[1] * finalCenter[1] +
                  finalCenter[2] * finalCenter[2]);
    EXPECT_LE(result.maxAbsEnergyDriftPct, 0.01f);
    EXPECT_GE(orbitRatio, 0.98f);
    EXPECT_LE(orbitRatio, 1.02f);
    EXPECT_LE(centerMagnitude, 1e-3f);
}
} // namespace testsupport
