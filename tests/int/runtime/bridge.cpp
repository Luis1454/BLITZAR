#include "frontend/FrontendBackendBridge.hpp"
#include "tests/support/poll_utils.hpp"
#include "tests/support/backend_harness.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace grav_test_frontend_bridge {

TEST(FrontendBridgeTest, TST_INT_RUNT_001_ReconnectsAfterRealBackendRestart)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;
    const std::uint16_t fixedPort = backend.port();
    ASSERT_NE(fixedPort, 0u);

    grav_frontend::FrontendBackendBridge bridge(
        "simulation.ini",
        true,
        "127.0.0.1",
        fixedPort,
        false,
        backend.executablePath(),
        "",
        80u,
        40u,
        120u,
        {});

    ASSERT_TRUE(bridge.start());

    SimulationStats stats{};
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        stats = bridge.getStats();
        return bridge.linkState() == grav_frontend::FrontendLinkState::Connected;
    }, std::chrono::milliseconds(4000)));

    bridge.stepOnce();
    EXPECT_EQ(bridge.linkState(), grav_frontend::FrontendLinkState::Connected);

    std::vector<RenderParticle> snapshot;
    EXPECT_TRUE(testsupport::waitUntil([&]() {
        return bridge.tryConsumeSnapshot(snapshot);
    }, std::chrono::milliseconds(2000)));
    EXPECT_FALSE(snapshot.empty());

    backend.stop();
    EXPECT_TRUE(testsupport::waitUntil([&]() {
        (void)bridge.getStats();
        return bridge.linkState() == grav_frontend::FrontendLinkState::Reconnecting;
    }, std::chrono::milliseconds(4000)));

    ASSERT_TRUE(backend.start(startError, fixedPort)) << startError;
    ASSERT_TRUE(testsupport::waitUntil([&]() {
        stats = bridge.getStats();
        return bridge.linkState() == grav_frontend::FrontendLinkState::Connected;
    }, std::chrono::milliseconds(5000)));

    bridge.stepOnce();
    EXPECT_EQ(bridge.linkState(), grav_frontend::FrontendLinkState::Connected);

    bridge.stop();
    backend.stop();
}

TEST(FrontendBridgeTest, TST_INT_RUNT_002_BackendAbsenceDoesNotCauseLongBlockingLoops)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;
    const std::uint16_t unusedPort = backend.port();
    backend.stop();

    grav_frontend::FrontendBackendBridge bridge(
        "simulation.ini",
        true,
        "127.0.0.1",
        unusedPort,
        false,
        "",
        "",
        60u,
        30u,
        80u,
        {});
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

    EXPECT_EQ(bridge.linkState(), grav_frontend::FrontendLinkState::Reconnecting);
    EXPECT_LE(elapsedMs.count(), 1500) << "poll loop took too long without backend: " << elapsedMs.count() << " ms";

    bridge.stop();
}

} // namespace grav_test_frontend_bridge



