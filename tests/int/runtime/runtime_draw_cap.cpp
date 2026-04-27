// File: tests/int/runtime/runtime_draw_cap.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "client/ClientCommon.hpp"
#include "config/SimulationConfig.hpp"
#include "protocol/ServerProtocol.hpp"
#include "tests/support/scoped_env_var.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
namespace grav_test_client_runtime {
/// Description: Executes the TEST operation.
TEST(ClientRuntimeTest, TST_CNT_RUNT_005_ClampsConfiguredClientCapToProtocolMax)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.clientParticleCap = grav_protocol::kSnapshotMaxPoints + 5000u;
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), grav_protocol::kSnapshotMaxPoints);
}
/// Description: Executes the TEST operation.
TEST(ClientRuntimeTest, TST_CNT_RUNT_006_ClampsEnvironmentOverrideToProtocolMax)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.clientParticleCap = 4096u;
    /// Description: Executes the drawCapOverride operation.
    testsupport::ScopedEnvVar drawCapOverride("GRAVITY_CLIENT_DRAW_CAP", "50000");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), grav_protocol::kSnapshotMaxPoints);
}
/// Description: Executes the TEST operation.
TEST(ClientRuntimeTest, TST_CNT_RUNT_007_InvalidEnvironmentOverrideFallsBackToConfigAndWarns)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.clientParticleCap = 4096u;
    std::stringstream err;
    std::streambuf* previous = std::cerr.rdbuf(err.rdbuf());
    /// Description: Executes the drawCapOverride operation.
    testsupport::ScopedEnvVar drawCapOverride("GRAVITY_CLIENT_DRAW_CAP", "bad");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), 4096u);
    std::cerr.rdbuf(previous);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(err.str().find("invalid GRAVITY_CLIENT_DRAW_CAP"), std::string::npos);
}
} // namespace grav_test_client_runtime
