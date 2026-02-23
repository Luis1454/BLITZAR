#include "sim/FrontendBackendBridge.hpp"
#include "tests/integration/RealBackendHarness.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace {

bool waitUntil(const std::function<bool()> &predicate, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return predicate();
}

TEST(FrontendBackendBridgeRegression, ReconnectsAfterRealBackendRestart)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;
    const std::uint16_t fixedPort = backend.port();
    ASSERT_NE(fixedPort, 0u);

    sim::FrontendBackendBridge bridge(
        "simulation.ini",
        true,
        "127.0.0.1",
        fixedPort,
        false,
        backend.executablePath(),
        80u,
        40u,
        120u,
        {});

    ASSERT_TRUE(bridge.start());

    SimulationStats stats{};
    ASSERT_TRUE(waitUntil([&]() {
        stats = bridge.getStats();
        return bridge.linkState() == sim::FrontendLinkState::Connected;
    }, std::chrono::milliseconds(4000)));

    bridge.stepOnce();
    EXPECT_EQ(bridge.linkState(), sim::FrontendLinkState::Connected);

    std::vector<RenderParticle> snapshot;
    EXPECT_TRUE(waitUntil([&]() {
        return bridge.tryConsumeSnapshot(snapshot);
    }, std::chrono::milliseconds(2000)));
    EXPECT_FALSE(snapshot.empty());

    backend.stop();
    EXPECT_TRUE(waitUntil([&]() {
        (void)bridge.getStats();
        return bridge.linkState() == sim::FrontendLinkState::Reconnecting;
    }, std::chrono::milliseconds(4000)));

    ASSERT_TRUE(backend.start(startError, fixedPort)) << startError;
    ASSERT_TRUE(waitUntil([&]() {
        stats = bridge.getStats();
        return bridge.linkState() == sim::FrontendLinkState::Connected;
    }, std::chrono::milliseconds(5000)));

    bridge.stepOnce();
    EXPECT_EQ(bridge.linkState(), sim::FrontendLinkState::Connected);

    bridge.stop();
    backend.stop();
}

TEST(FrontendBackendBridgeRegression, BackendAbsenceDoesNotCauseLongBlockingLoops)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;
    const std::uint16_t unusedPort = backend.port();
    backend.stop();

    sim::FrontendBackendBridge bridge(
        "simulation.ini",
        true,
        "127.0.0.1",
        unusedPort,
        false,
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

    EXPECT_EQ(bridge.linkState(), sim::FrontendLinkState::Reconnecting);
    EXPECT_LE(elapsedMs.count(), 1500) << "poll loop took too long without backend: " << elapsedMs.count() << " ms";

    bridge.stop();
}

} // namespace
