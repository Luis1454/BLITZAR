/*
 * @file tests/unit/ui/qt_throughput_advisor.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "config/SimulationConfig.hpp"
#include "ui/ThroughputAdvisor.hpp"
#include <gtest/gtest.h>

namespace grav_test_qt_throughput_advisor {
TEST(QtUiLogicTest, TST_UNT_UI_006_ThroughputAdvisorWarnsOnHeavyPairwiseConfig)
{
    SimulationConfig config{};
    config.solver = "pairwise_cuda";
    config.particleCount = 100000u;
    config.dt = 0.1f;
    config.substepTargetDt = 0.01f;
    config.maxSubsteps = 6u;
    config.clientParticleCap = 100000u;
    const grav_qt::ThroughputAdvisory advisory =
        grav_qt::ThroughputAdvisor::evaluate(config, config.clientParticleCap);
    EXPECT_EQ(advisory.severity, grav_qt::ThroughputAdvisorySeverity::Warning);
    EXPECT_LT(advisory.estimatedStepsPerSecond, 1.0f);
    EXPECT_EQ(advisory.estimatedSubsteps, 6u);
    EXPECT_NE(advisory.summary.find("pairwise_cuda"), std::string::npos);
    EXPECT_NE(advisory.action.find("octree_gpu"), std::string::npos);
}

TEST(QtUiLogicTest, TST_UNT_UI_007_ThroughputAdvisorStaysQuietForInteractiveOctreeConfig)
{
    SimulationConfig config{};
    config.solver = "octree_gpu";
    config.particleCount = 20000u;
    config.dt = 0.01f;
    config.substepTargetDt = 0.01f;
    config.maxSubsteps = 4u;
    config.clientParticleCap = 4096u;
    const grav_qt::ThroughputAdvisory advisory =
        grav_qt::ThroughputAdvisor::evaluate(config, config.clientParticleCap);
    EXPECT_EQ(advisory.severity, grav_qt::ThroughputAdvisorySeverity::None);
    EXPECT_TRUE(advisory.summary.empty());
    EXPECT_TRUE(advisory.action.empty());
}
} // namespace grav_test_qt_throughput_advisor
