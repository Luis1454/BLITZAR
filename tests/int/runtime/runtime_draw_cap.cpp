/*
 * @file tests/int/runtime/runtime_draw_cap.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "client/ClientCommon.hpp"
#include "config/SimulationConfig.hpp"
#include "protocol/ServerProtocol.hpp"
#include "tests/support/scoped_env_var.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

namespace bltzr_test_client_runtime {
TEST(ClientRuntimeTest, TST_CNT_RUNT_005_ClampsConfiguredClientCapToProtocolMax)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.clientParticleCap = bltzr_protocol::kSnapshotMaxPoints + 5000u;
    EXPECT_EQ(bltzr_client::resolveClientDrawCap(config), bltzr_protocol::kSnapshotMaxPoints);
}

TEST(ClientRuntimeTest, TST_CNT_RUNT_006_ClampsEnvironmentOverrideToProtocolMax)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.clientParticleCap = 4096u;
    testsupport::ScopedEnvVar drawCapOverride("BLITZAR_CLIENT_DRAW_CAP", "50000");
    EXPECT_EQ(bltzr_client::resolveClientDrawCap(config), bltzr_protocol::kSnapshotMaxPoints);
}

TEST(ClientRuntimeTest, TST_CNT_RUNT_007_InvalidEnvironmentOverrideFallsBackToConfigAndWarns)
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
