#include "client/ClientRuntime.hpp"
#include "tests/support/client_utils.hpp"
#include "tests/support/poll_utils.hpp"
#include "tests/support/server_harness.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace grav_test_client_runtime {

TEST(ClientRuntimeTest, TST_CNT_RUNT_001_ConnectsToRealServerAndPublishesStatsAndSnapshot)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;

    grav_client::ClientRuntime runtime("simulation.ini", testsupport::makeTransport(server.port(), server.executablePath()));
    ASSERT_TRUE(runtime.start());

    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(4000)));

    const SimulationStats stats = runtime.getStats();
    EXPECT_GT(stats.particleCount, 0u);
    EXPECT_GT(stats.dt, 0.0f);
    EXPECT_FALSE(stats.solverName.empty());
    EXPECT_FALSE(stats.faulted);

    std::optional<grav_client::ConsumedSnapshot> consumedSnapshot;
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        consumedSnapshot = runtime.consumeLatestSnapshot();
        return consumedSnapshot.has_value();
    }, std::chrono::milliseconds(2500)));
    ASSERT_TRUE(consumedSnapshot.has_value());
    std::vector<RenderParticle> snapshot = std::move(consumedSnapshot->particles);
    EXPECT_FALSE(snapshot.empty());
    EXPECT_EQ(consumedSnapshot->sourceSize, snapshot.size());

    EXPECT_NE(runtime.statsAgeMs(), std::numeric_limits<std::uint32_t>::max());
    EXPECT_NE(runtime.snapshotAgeMs(), std::numeric_limits<std::uint32_t>::max());

    runtime.stop();
    server.stop();
}

TEST(ClientRuntimeTest, TST_CNT_RUNT_002_StartFailsWhenRemoteServerIsUnavailable)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    const std::uint16_t unavailablePort = server.port();
    server.stop();

    grav_client::ClientRuntime runtime("simulation.ini", testsupport::makeTransport(unavailablePort, ""));
    EXPECT_FALSE(runtime.start());
    EXPECT_EQ(runtime.linkStateLabel(), "reconnecting");
}

TEST(ClientRuntimeTest, TST_CNT_RUNT_003_ReconnectsWhenRealServerRestarts)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    const std::uint16_t fixedPort = server.port();

    grav_client::ClientRuntime runtime("simulation.ini", testsupport::makeTransport(fixedPort, server.executablePath()));
    ASSERT_TRUE(runtime.start());

    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(4000)));

    server.stop();
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "reconnecting";
    }, std::chrono::milliseconds(4000)));

    ASSERT_TRUE(server.start(startError, fixedPort)) << startError;
    runtime.requestReconnect();
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(5000)));

    runtime.stop();
    server.stop();
}

TEST(ClientRuntimeTest, TST_CNT_RUNT_004_ConnectorCanBeReconfiguredAtRuntimeToReachRealServer)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;

    grav_client::ClientRuntime runtime("simulation.ini", testsupport::makeTransport(server.port(), server.executablePath()));
    ASSERT_TRUE(runtime.start());

    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(4000)));

    const std::uint16_t wrongPort = static_cast<std::uint16_t>(
        server.port() == 65535u ? server.port() - 1u : server.port() + 1u);
    runtime.configureRemoteConnector("127.0.0.1", wrongPort, false, server.executablePath());
    runtime.requestReconnect();
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "reconnecting";
    }, std::chrono::milliseconds(3000)));

    runtime.configureRemoteConnector("127.0.0.1", server.port(), false, server.executablePath());
    runtime.requestReconnect();
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(5000)));

    EXPECT_TRUE(runtime.isRemoteMode());
    EXPECT_EQ(runtime.serverOwnerLabel(), "external");

    runtime.stop();
    server.stop();
}

} // namespace grav_test_client_runtime



