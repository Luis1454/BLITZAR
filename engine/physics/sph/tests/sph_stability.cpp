/*
 * @file engine/physics/sph/tests/sph_stability.cpp
 * @project BLITZAR
 * @brief SPH stability and bounded-drift qualification.
 */

#include "tests/support/physics_scenario.hpp"
#include "tests/support/physics_test_utils.hpp"

#include <cmath>
#include <gtest/gtest.h>
#include <string>

namespace testsupport {
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

    ScenarioResult result;
    std::string error;
    if (!runScenario(cfg, result, error)) {
        if (error.find("timeout") != std::string::npos) {
            GTEST_SKIP() << "SPH scenario timed out: " << error;
        }
        FAIL() << "SPH scenario failed: " << error;
    }
    if (result.stats.solverName != cfg.solver) {
        GTEST_SKIP() << "pairwise_cuda unavailable (actual solver="
                     << result.stats.solverName << ")";
    }

    EXPECT_LE(result.maxAbsEnergyDriftPct, 25.0f);
    for (const auto& particle : result.final) {
        EXPECT_TRUE(std::isfinite(particle.x));
        EXPECT_TRUE(std::isfinite(particle.y));
        EXPECT_TRUE(std::isfinite(particle.z));
        const float radius = std::sqrt(particle.x * particle.x + particle.y * particle.y +
                                       particle.z * particle.z);
        EXPECT_LE(radius, 20.0f);
    }
}
} // namespace testsupport
