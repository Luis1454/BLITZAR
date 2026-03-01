#include "tests/support/physics_scenario.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace testsupport {
TEST_F(PhysicsTest, TST_UNT_PHYS_001_AttractionDistance)
{
    ScenarioConfig cfg;
    cfg.inputPath = inputPath_;
    cfg.particleCount = 2u;
    cfg.initState.mode = "file";
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 0.0f;
    cfg.dt = 0.002f;
    cfg.steps = 400;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;

    const float initialDistance = distance(result.initial[0], result.initial[1]);
    const float finalDistance = distance(result.final[0], result.final[1]);
    const float ratio = finalDistance / std::max(initialDistance, 1e-8f);

    constexpr float kMinRatio = 0.05f;
    constexpr float kMaxRatio = 0.999f;
    EXPECT_GE(ratio, kMinRatio) << "Distance collapsed too much";
    EXPECT_LE(ratio, kMaxRatio) << "No attraction detected";
}

TEST_F(PhysicsTest, TST_UNT_PHYS_003_CenterOfMassDrift)
{
    ScenarioConfig cfg;
    cfg.inputPath = inputPath_;
    cfg.particleCount = 2u;
    cfg.initState.mode = "file";
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 0.0f;
    cfg.dt = 0.002f;
    cfg.steps = 400;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;

    const auto centerOfMass = [](const std::vector<RenderParticle> &snapshot) {
        const float totalMass = snapshot[0].mass + snapshot[1].mass;
        const float cx = (snapshot[0].x * snapshot[0].mass + snapshot[1].x * snapshot[1].mass) / totalMass;
        const float cy = (snapshot[0].y * snapshot[0].mass + snapshot[1].y * snapshot[1].mass) / totalMass;
        const float cz = (snapshot[0].z * snapshot[0].mass + snapshot[1].z * snapshot[1].mass) / totalMass;
        return std::array<float, 3>{cx, cy, cz};
    };

    const auto c0 = centerOfMass(result.initial);
    const auto c1 = centerOfMass(result.final);
    const float drift = std::sqrt(
        (c1[0] - c0[0]) * (c1[0] - c0[0])
        + (c1[1] - c0[1]) * (c1[1] - c0[1])
        + (c1[2] - c0[2]) * (c1[2] - c0[2]));

    constexpr float kMaxCenterOfMassDrift = 1e-2f;
    EXPECT_LE(drift, kMaxCenterOfMassDrift);
}

TEST_F(PhysicsTest, TST_UNT_PHYS_004_TimeStepConvergence)
{
    ScenarioConfig coarse;
    coarse.inputPath = inputPath_;
    coarse.particleCount = 2u;
    coarse.initState.mode = "file";
    coarse.initState.thermalAmbientTemperature = 0.0f;
    coarse.initState.thermalSpecificHeat = 1.0f;
    coarse.initState.thermalHeatingCoeff = 0.0f;
    coarse.initState.thermalRadiationCoeff = 0.0f;
    coarse.dt = 0.002f;
    coarse.steps = 100;

    ScenarioConfig fine;
    fine.inputPath = inputPath_;
    fine.particleCount = 2u;
    fine.initState.mode = "file";
    fine.initState.thermalAmbientTemperature = 0.0f;
    fine.initState.thermalSpecificHeat = 1.0f;
    fine.initState.thermalHeatingCoeff = 0.0f;
    fine.initState.thermalRadiationCoeff = 0.0f;
    fine.dt = 0.001f;
    fine.steps = 200;

    ScenarioResult coarseResult;
    std::string coarseError;
    ASSERT_TRUE(runScenario(coarse, coarseResult, coarseError)) << coarseError;

    ScenarioResult fineResult;
    std::string fineError;
    ASSERT_TRUE(runScenario(fine, fineResult, fineError)) << fineError;

    float maxParticleDelta = 0.0f;
    for (std::size_t i = 0; i < 2; ++i) {
        maxParticleDelta = std::max(maxParticleDelta, distance(coarseResult.final[i], fineResult.final[i]));
    }

    constexpr float kMaxConvergenceDelta = 0.05f;
    EXPECT_LE(maxParticleDelta, kMaxConvergenceDelta);
}

TEST_F(PhysicsTest, TST_UNT_PHYS_002_EnergyConservation)
{
    ScenarioConfig cfg;
    cfg.inputPath = inputPath_;
    cfg.particleCount = 2u;
    cfg.initState.mode = "file";
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 0.0f;
    cfg.dt = 0.002f;
    cfg.steps = 800;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;

    constexpr float kMaxEnergyDriftPct = 5.0f;
    EXPECT_LE(result.maxAbsEnergyDriftPct, kMaxEnergyDriftPct)
        << "Energy drift too high: " << result.maxAbsEnergyDriftPct << "%";
    EXPECT_FALSE(result.stats.energyEstimated);
}

TEST_F(PhysicsTest, TST_UNT_PHYS_005_LongRunStability)
{
    ScenarioConfig cfg;
    cfg.inputPath = inputPath_;
    cfg.particleCount = 2u;
    cfg.initState.mode = "file";
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 0.0f;
    cfg.dt = 0.001f;
    cfg.steps = 2000;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;

    ASSERT_FALSE(result.final.empty());
    for (const RenderParticle &p : result.final) {
        EXPECT_TRUE(std::isfinite(p.x));
        EXPECT_TRUE(std::isfinite(p.y));
        EXPECT_TRUE(std::isfinite(p.z));
    }

    float maxRadius = 0.0f;
    for (const RenderParticle &p : result.final) {
        const float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        maxRadius = std::max(maxRadius, r);
    }
    constexpr float kMaxStableRadius = 50.0f;
    EXPECT_LE(maxRadius, kMaxStableRadius) << "Trajectory escaped expected stable bounds";
}

} // namespace testsupport

