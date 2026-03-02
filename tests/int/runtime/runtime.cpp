#include "frontend/FrontendRuntime.hpp"
#include "tests/support/frontend_utils.hpp"
#include "tests/support/poll_utils.hpp"
#include "tests/support/backend_harness.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace grav_test_frontend_runtime {

TEST(FrontendRuntimeTest, TST_CNT_RUNT_001_ConnectsToRealBackendAndPublishesStatsAndSnapshot)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    grav_frontend::FrontendRuntime runtime("simulation.ini", testsupport::makeTransport(backend.port(), backend.executablePath()));
    ASSERT_TRUE(runtime.start());

    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(4000)));

    const SimulationStats stats = runtime.getStats();
    EXPECT_GT(stats.particleCount, 0u);
    EXPECT_GT(stats.dt, 0.0f);
    EXPECT_FALSE(stats.solverName.empty());
    EXPECT_FALSE(stats.faulted);

    std::vector<RenderParticle> snapshot;
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.tryConsumeSnapshot(snapshot);
    }, std::chrono::milliseconds(2500)));
    EXPECT_FALSE(snapshot.empty());

    EXPECT_NE(runtime.statsAgeMs(), std::numeric_limits<std::uint32_t>::max());
    EXPECT_NE(runtime.snapshotAgeMs(), std::numeric_limits<std::uint32_t>::max());

    runtime.stop();
    backend.stop();
}

TEST(FrontendRuntimeTest, TST_CNT_RUNT_002_StartFailsWhenRemoteBackendIsUnavailable)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;
    const std::uint16_t unavailablePort = backend.port();
    backend.stop();

    grav_frontend::FrontendRuntime runtime("simulation.ini", testsupport::makeTransport(unavailablePort, ""));
    EXPECT_FALSE(runtime.start());
    EXPECT_EQ(runtime.linkStateLabel(), "reconnecting");
}

TEST(FrontendRuntimeTest, TST_CNT_RUNT_003_ReconnectsWhenRealBackendRestarts)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;
    const std::uint16_t fixedPort = backend.port();

    grav_frontend::FrontendRuntime runtime("simulation.ini", testsupport::makeTransport(fixedPort, backend.executablePath()));
    ASSERT_TRUE(runtime.start());

    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(4000)));

    backend.stop();
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "reconnecting";
    }, std::chrono::milliseconds(4000)));

    ASSERT_TRUE(backend.start(startError, fixedPort)) << startError;
    runtime.requestReconnect();
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(5000)));

    runtime.stop();
    backend.stop();
}

TEST(FrontendRuntimeTest, TST_CNT_RUNT_004_ConnectorCanBeReconfiguredAtRuntimeToReachRealBackend)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    grav_frontend::FrontendRuntime runtime("simulation.ini", testsupport::makeTransport(backend.port(), backend.executablePath()));
    ASSERT_TRUE(runtime.start());

    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(4000)));

    const std::uint16_t wrongPort = static_cast<std::uint16_t>(
        backend.port() == 65535u ? backend.port() - 1u : backend.port() + 1u);
    runtime.configureRemoteConnector("127.0.0.1", wrongPort, false, backend.executablePath());
    runtime.requestReconnect();
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "reconnecting";
    }, std::chrono::milliseconds(3000)));

    runtime.configureRemoteConnector("127.0.0.1", backend.port(), false, backend.executablePath());
    runtime.requestReconnect();
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(5000)));

    EXPECT_TRUE(runtime.isRemoteMode());
    EXPECT_EQ(runtime.backendOwnerLabel(), "external");

    runtime.stop();
    backend.stop();
}

} // namespace grav_test_frontend_runtime



