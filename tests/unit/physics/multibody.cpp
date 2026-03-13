#include "tests/support/physics_scenario.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace testsupport {
TEST(PhysicsTest, TST_UNT_PHYS_007_MultiBodyInteractions)
{
    ScenarioConfig cfg;
    cfg.particleCount = 256u;
    cfg.dt = 0.05f;
    cfg.steps = 8;
    cfg.solver = "octree_cpu";
    cfg.integrator = "euler";
    cfg.energyMeasureEverySteps = 100u;
    cfg.energySampleLimit = 512u;
    cfg.snapshotTimeoutMs = 10000;
    cfg.stepTimeoutMs = 12000;
    cfg.initState.mode = "disk_orbit";
    cfg.initState.seed = 42u;
    cfg.initState.includeCentralBody = true;
    cfg.initState.centralMass = 1.0f;
    cfg.initState.diskMass = 1000000.0f;
    cfg.initState.diskRadiusMin = 1.5f;
    cfg.initState.diskRadiusMax = 11.5f;
    cfg.initState.diskThickness = 0.0f;
    cfg.initState.velocityScale = 1.0f;
    cfg.initState.velocityTemperature = 2.0f;
    cfg.initState.particleTemperature = 0.0f;
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0002f;
    cfg.initState.thermalRadiationCoeff = 0.00000001f;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    ASSERT_EQ(result.initial.size(), result.final.size());
    ASSERT_GE(result.initial.size(), 256u);

    std::size_t movedParticles = 0u;
    float totalDisplacement = 0.0f;
    float maxDisplacement = 0.0f;
    for (std::size_t i = 0; i < result.final.size(); ++i) {
        EXPECT_TRUE(std::isfinite(result.final[i].x));
        EXPECT_TRUE(std::isfinite(result.final[i].y));
        EXPECT_TRUE(std::isfinite(result.final[i].z));
        const float dx = result.final[i].x - result.initial[i].x;
        const float dy = result.final[i].y - result.initial[i].y;
        const float dz = result.final[i].z - result.initial[i].z;
        const float disp = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (disp > 1e-4f) {
            ++movedParticles;
        }
        totalDisplacement += disp;
        maxDisplacement = std::max(maxDisplacement, disp);
    }

    const float avgDisplacement = totalDisplacement / static_cast<float>(result.final.size());
    EXPECT_GE(movedParticles, result.final.size() * 3u / 5u);
    EXPECT_GT(avgDisplacement, 4e-4f);
    EXPECT_GT(maxDisplacement, 2e-3f);

    float maxRadius = 0.0f;
    for (const RenderParticle &p : result.final) {
        const float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        maxRadius = std::max(maxRadius, r);
    }
    constexpr float kMaxStableRadius = 100.0f;
    EXPECT_LE(maxRadius, kMaxStableRadius);
}

TEST(PhysicsTest, TST_UNT_PHYS_006_EnergyConservationHighMassNoSph)
{
    ScenarioConfig cfg;
    cfg.particleCount = 96u;
    cfg.dt = 0.1f;
    cfg.steps = 12;
    cfg.solver = "octree_cpu";
    cfg.integrator = "euler";
    cfg.energyMeasureEverySteps = 1u;
    cfg.energySampleLimit = 96u;
    cfg.snapshotTimeoutMs = 6000;
    cfg.stepTimeoutMs = 6000;
    cfg.initState.mode = "disk_orbit";
    cfg.initState.seed = 42u;
    cfg.initState.includeCentralBody = true;
    cfg.initState.centralMass = 1.0f;
    cfg.initState.diskMass = 1000000.0f;
    cfg.initState.diskRadiusMin = 1.5f;
    cfg.initState.diskRadiusMax = 11.5f;
    cfg.initState.diskThickness = 0.0f;
    cfg.initState.velocityScale = 1.0f;
    cfg.initState.velocityTemperature = 2.0f;
    cfg.initState.particleTemperature = 0.0f;
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 0.0f;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    EXPECT_FALSE(result.stats.energyEstimated);

    constexpr float kMaxEnergyDriftPct = 8.0f;
    EXPECT_LE(result.maxAbsEnergyDriftPct, kMaxEnergyDriftPct)
        << "High-mass no-SPH drift too high: " << result.maxAbsEnergyDriftPct << "%";
}

TEST(PhysicsTest, TST_UNT_PHYS_008_RadiationExchangeConservation)
{
    ScenarioConfig cfg;
    cfg.particleCount = 48u;
    cfg.dt = 0.1f;
    cfg.steps = 16;
    cfg.solver = "octree_cpu";
    cfg.integrator = "euler";
    cfg.energyMeasureEverySteps = 1u;
    cfg.energySampleLimit = 64u;
    cfg.snapshotTimeoutMs = 8000;
    cfg.stepTimeoutMs = 8000;
    cfg.initState.mode = "random_cloud";
    cfg.initState.seed = 7u;
    cfg.initState.includeCentralBody = false;
    cfg.initState.cloudHalfExtent = 50.0f;
    cfg.initState.cloudSpeed = 0.0f;
    cfg.initState.particleMass = 1e-6f;
    cfg.initState.velocityTemperature = 0.0f;
    cfg.initState.particleTemperature = 1000.0f;
    cfg.initState.thermalAmbientTemperature = 0.0f;
    cfg.initState.thermalSpecificHeat = 1.0f;
    cfg.initState.thermalHeatingCoeff = 0.0f;
    cfg.initState.thermalRadiationCoeff = 1.0f;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    EXPECT_FALSE(result.stats.energyEstimated);

    float initialThermal = 0.0f;
    for (const RenderParticle &p : result.initial) {
        initialThermal += p.mass * std::max(0.0f, p.temperature);
    }

    constexpr float kMaxEnergyDriftPct = 1.5f;
    EXPECT_LE(result.maxAbsEnergyDriftPct, kMaxEnergyDriftPct)
        << "Radiative exchange should conserve total (thermal + radiated) energy";
    EXPECT_GT(result.stats.radiatedEnergy, 0.0f);
    EXPECT_LT(result.stats.thermalEnergy, initialThermal);
}

TEST(PhysicsTest, TST_UNT_PHYS_011_CalibrationThreeBodyPresetStaysFiniteAndCentered)
{
    ScenarioConfig cfg;
    std::string error;
    ASSERT_TRUE(prepareGeneratedCalibrationScenario("three_body", cfg, error)) << error;
    cfg.solver = "octree_cpu";

    ScenarioResult result;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    ASSERT_EQ(result.final.size(), 3u);

    float maxRadius = 0.0f;
    for (const RenderParticle &particle : result.final) {
        EXPECT_TRUE(std::isfinite(particle.x));
        EXPECT_TRUE(std::isfinite(particle.y));
        EXPECT_TRUE(std::isfinite(particle.z));
        const float radius = std::sqrt(
            particle.x * particle.x + particle.y * particle.y + particle.z * particle.z);
        maxRadius = std::max(maxRadius, radius);
    }

    const auto finalCenter = centerOfMassAll(result.final);
    const float centerMagnitude = std::sqrt(
        finalCenter[0] * finalCenter[0] + finalCenter[1] * finalCenter[1] + finalCenter[2] * finalCenter[2]);
    EXPECT_LE(result.maxAbsEnergyDriftPct, 0.01f);
    EXPECT_LE(averageRadius(result.final), 2.0f);
    EXPECT_LE(maxRadius, 2.5f);
    EXPECT_LE(centerMagnitude, 1e-3f);
}

TEST(PhysicsTest, TST_UNT_PHYS_012_CalibrationPlummerPresetProducesBoundCluster)
{
    ScenarioConfig cfg;
    std::string error;
    ASSERT_TRUE(prepareGeneratedCalibrationScenario("plummer_sphere", cfg, error)) << error;
    cfg.solver = "octree_cpu";

    ScenarioResult result;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    ASSERT_EQ(result.initial.size(), 96u);
    ASSERT_EQ(result.final.size(), 96u);

    float maxRadius = 0.0f;
    for (const RenderParticle &particle : result.final) {
        EXPECT_TRUE(std::isfinite(particle.x));
        EXPECT_TRUE(std::isfinite(particle.y));
        EXPECT_TRUE(std::isfinite(particle.z));
        const float radius = std::sqrt(
            particle.x * particle.x + particle.y * particle.y + particle.z * particle.z);
        maxRadius = std::max(maxRadius, radius);
    }

    const auto finalCenter = centerOfMassAll(result.final);
    const float centerMagnitude = std::sqrt(
        finalCenter[0] * finalCenter[0] + finalCenter[1] * finalCenter[1] + finalCenter[2] * finalCenter[2]);
    const float finalAverageRadius = averageRadius(result.final);
    EXPECT_LE(result.maxAbsEnergyDriftPct, 0.01f);
    EXPECT_GE(finalAverageRadius, 4.0f);
    EXPECT_LE(finalAverageRadius, 12.0f);
    EXPECT_LE(maxRadius, 45.0f);
    EXPECT_LE(centerMagnitude, 0.02f);
}

} // namespace testsupport

