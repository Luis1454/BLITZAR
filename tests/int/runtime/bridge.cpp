#include "client/ClientServerBridge.hpp"
#include "tests/support/poll_utils.hpp"
#include "tests/support/server_harness.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace grav_test_client_bridge {

TEST(ClientBridgeTest, TST_INT_RUNT_001_ReconnectsAfterRealServerRestart)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    const std::uint16_t fixedPort = server.port();
    ASSERT_NE(fixedPort, 0u);

    grav_client::ClientServerBridge bridge(
        "simulation.ini",
        "127.0.0.1",
        fixedPort,
        false,
        server.executablePath(),
        "",
        80u,
        40u,
        120u);

    ASSERT_TRUE(bridge.start());

    SimulationStats stats{};
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        stats = bridge.getStats();
        return bridge.linkState() == grav_client::ClientLinkState::Connected;
    }, std::chrono::milliseconds(4000)));

    bridge.stepOnce();
    EXPECT_EQ(bridge.linkState(), grav_client::ClientLinkState::Connected);

    std::vector<RenderParticle> snapshot;
    EXPECT_TRUE(testsupport::waitUntil([&]() {
        return bridge.tryConsumeSnapshot(snapshot);
    }, std::chrono::milliseconds(2000)));
    EXPECT_FALSE(snapshot.empty());

    server.stop();
    EXPECT_TRUE(testsupport::waitUntil([&]() {
        (void)bridge.getStats();
        return bridge.linkState() == grav_client::ClientLinkState::Reconnecting;
    }, std::chrono::milliseconds(4000)));

    ASSERT_TRUE(server.start(startError, fixedPort)) << startError;
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        stats = bridge.getStats();
        return bridge.linkState() == grav_client::ClientLinkState::Connected;
    }, std::chrono::milliseconds(5000)));

    bridge.stepOnce();
    EXPECT_EQ(bridge.linkState(), grav_client::ClientLinkState::Connected);

    bridge.stop();
    server.stop();
}

TEST(ClientBridgeTest, TST_INT_RUNT_002_ServerAbsenceDoesNotCauseLongBlockingLoops)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    const std::uint16_t unusedPort = server.port();
    server.stop();

    grav_client::ClientServerBridge bridge(
        "simulation.ini",
        "127.0.0.1",
        unusedPort,
        false,
        "",
        "",
        60u,
        30u,
        80u);
    EXPECT_FALSE(bridge.start());

    const auto startedAt = std::chrono::steady_clock::now();
    for (int i = 0; i < 24; ++i) {
        (void)bridge.getStats();
        std::vector<RenderParticle> snapshot;
        (void)bridge.tryConsumeSnapshot(snapshot);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startedAt);

    EXPECT_EQ(bridge.linkState(), grav_client::ClientLinkState::Reconnecting);
    EXPECT_LE(elapsedMs.count(), 1500) << "poll loop took too long without server: " << elapsedMs.count() << " ms";

    bridge.stop();
}

} // namespace grav_test_client_bridge



