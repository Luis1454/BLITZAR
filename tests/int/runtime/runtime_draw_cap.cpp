/*
 * @file tests/int/runtime/runtime_draw_cap.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "client/common/ClientCommon.hpp"
#include "config/core/CfgConfig.hpp"
#include "protocol/PtcProtocol.hpp"
#include "tests/support/scoped_env_var.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <limits>
#include <sstream>

namespace bltzr_test_client_runtime {
TEST(RuntimeTest, TST_CNT_RUNT_005_AcceptsMaximumConfiguredClientCap)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.clientParticleCap = std::numeric_limits<std::uint32_t>::max();
    EXPECT_EQ(bltzr_client::resolveClientDrawCap(config), bltzr_protocol::kSnapshotMaxPoints);
}

TEST(RuntimeTest, TST_CNT_RUNT_006_ClampsEnvironmentOverrideToProtocolMax)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.clientParticleCap = 4096u;
    testsupport::ScopedEnvVar drawCapOverride("BLITZAR_CLIENT_DRAW_CAP", "5000000");
    EXPECT_EQ(bltzr_client::resolveClientDrawCap(config), bltzr_protocol::kSnapshotMaxPoints);
}

TEST(RuntimeTest, TST_CNT_RUNT_007_InvalidEnvironmentOverrideFallsBackToConfigAndWarns)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.clientParticleCap = 4096u;
    std::stringstream err;
    std::streambuf* previous = std::cerr.rdbuf(err.rdbuf());
    testsupport::ScopedEnvVar drawCapOverride("BLITZAR_CLIENT_DRAW_CAP", "bad");
    EXPECT_EQ(bltzr_client::resolveClientDrawCap(config), 4096u);
    std::cerr.rdbuf(previous);
    EXPECT_NE(err.str().find("invalid BLITZAR_CLIENT_DRAW_CAP"), std::string::npos);
}
} // namespace bltzr_test_client_runtime
