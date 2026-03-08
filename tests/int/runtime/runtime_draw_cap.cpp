#include "frontend/FrontendCommon.hpp"
#include "protocol/BackendProtocol.hpp"
#include "tests/support/scoped_env_var.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <sstream>

namespace grav_test_frontend_runtime {

TEST(FrontendRuntimeTest, TST_CNT_RUNT_005_ClampsConfiguredFrontendCapToProtocolMax)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.frontendParticleCap = grav_protocol::kSnapshotMaxPoints + 5000u;

    EXPECT_EQ(grav_frontend::resolveFrontendDrawCap(config), grav_protocol::kSnapshotMaxPoints);
}

TEST(FrontendRuntimeTest, TST_CNT_RUNT_006_ClampsEnvironmentOverrideToProtocolMax)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.frontendParticleCap = 4096u;

    testsupport::ScopedEnvVar drawCapOverride("GRAVITY_FRONTEND_DRAW_CAP", "50000");
    EXPECT_EQ(grav_frontend::resolveFrontendDrawCap(config), grav_protocol::kSnapshotMaxPoints);
}

TEST(FrontendRuntimeTest, TST_CNT_RUNT_007_InvalidEnvironmentOverrideFallsBackToConfigAndWarns)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.frontendParticleCap = 4096u;

    std::stringstream err;
    std::streambuf *previous = std::cerr.rdbuf(err.rdbuf());
    testsupport::ScopedEnvVar drawCapOverride("GRAVITY_FRONTEND_DRAW_CAP", "bad");
    EXPECT_EQ(grav_frontend::resolveFrontendDrawCap(config), 4096u);
    std::cerr.rdbuf(previous);

    EXPECT_NE(err.str().find("invalid GRAVITY_FRONTEND_DRAW_CAP"), std::string::npos);
}

} // namespace grav_test_frontend_runtime
