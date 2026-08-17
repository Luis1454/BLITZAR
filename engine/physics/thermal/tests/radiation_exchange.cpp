/*
 * @file engine/physics/thermal/tests/radiation_exchange.cpp
 * @brief Thermal exchange regression coverage.
 */

#include "tests/support/physics_scenario.hpp"
#include "tests/support/physics_test_utils.hpp"

#include <algorithm>
#include <gtest/gtest.h>
#include <string>

namespace testsupport {
TEST(PhysicsTest, TST_UNT_PHYS_008_RadiationExchangeConservation)
{
    ScenarioConfig cfg = buildRandomCloudScenario(48u, 0.1f, 16u, 7u, "octree_cpu", "euler");
    setScenarioEnergySampling(cfg, 1u, 64u);
    setScenarioTiming(cfg, 8000, 8000);
    cfg.initState.cloudHalfExtent = 50.0f;
    cfg.initState.cloudSpeed = 0.0f;
    cfg.initState.particleMass = 1e-6f;
    cfg.initState.velocityTemperature = 0.0f;
    cfg.initState.particleTemperature = 1000.0f;
    cfg.initState.thermalRadiationCoeff = 1.0f;

    ScenarioResult result;
    std::string error;
    ASSERT_TRUE(runScenario(cfg, result, error)) << error;
    EXPECT_FALSE(result.stats.energyEstimated);

    float initialThermal = 0.0f;
    for (const RenderParticle& particle : result.initial) {
        initialThermal += particle.mass * std::max(0.0f, particle.temperature);
    }

    constexpr float kMaxEnergyDriftPct = 1.5f;
    EXPECT_LE(result.maxAbsEnergyDriftPct, kMaxEnergyDriftPct);
    EXPECT_GT(result.stats.radiatedEnergy, 0.0f);
    EXPECT_LT(result.stats.thermalEnergy, initialThermal);
}
} // namespace testsupport
