// File: tests/unit/ui/qt_throughput_advisor.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "config/SimulationConfig.hpp"
#include "ui/ThroughputAdvisor.hpp"
#include <gtest/gtest.h>
namespace grav_test_qt_throughput_advisor {
/// Description: Executes the TEST operation.
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
        /// Description: Executes the evaluate operation.
        grav_qt::ThroughputAdvisor::evaluate(config, config.clientParticleCap);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(advisory.severity, grav_qt::ThroughputAdvisorySeverity::Warning);
    /// Description: Executes the EXPECT_LT operation.
    EXPECT_LT(advisory.estimatedStepsPerSecond, 1.0f);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(advisory.estimatedSubsteps, 6u);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(advisory.summary.find("pairwise_cuda"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(advisory.action.find("octree_gpu"), std::string::npos);
}
/// Description: Executes the TEST operation.
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
        /// Description: Executes the evaluate operation.
        grav_qt::ThroughputAdvisor::evaluate(config, config.clientParticleCap);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(advisory.severity, grav_qt::ThroughputAdvisorySeverity::None);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(advisory.summary.empty());
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(advisory.action.empty());
}
} // namespace grav_test_qt_throughput_advisor
