#include "sim/FrontendRuntime.hpp"
#include "sim/ILocalBackend.hpp"
#include "tests/integration/RealBackendHarness.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace sim {

std::unique_ptr<ILocalBackend> createLocalBackend(const std::string &)
{
    return nullptr;
}

} // namespace sim

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

sim::FrontendTransportArgs makeTransport(std::uint16_t port, const std::string &backendExecutable)
{
    sim::FrontendTransportArgs transport{};
    transport.remoteMode = true;
    transport.remoteHost = "127.0.0.1";
    transport.remotePort = port;
    transport.remoteAutoStart = false;
    transport.backendExecutable = backendExecutable;
    transport.remoteCommandTimeoutMs = 80u;
    transport.remoteStatusTimeoutMs = 40u;
    transport.remoteSnapshotTimeoutMs = 120u;
    return transport;
}

TEST(FrontendRuntimeContractRegression, ConnectsToRealBackendAndPublishesStatsAndSnapshot)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    sim::FrontendRuntime runtime("simulation.ini", makeTransport(backend.port(), backend.executablePath()));
    ASSERT_TRUE(runtime.start());

    ASSERT_TRUE(waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(4000)));

    const SimulationStats stats = runtime.getStats();
    EXPECT_GT(stats.particleCount, 0u);
    EXPECT_GT(stats.dt, 0.0f);
    EXPECT_FALSE(stats.solverName.empty());
    EXPECT_FALSE(stats.faulted);

    std::vector<RenderParticle> snapshot;
    ASSERT_TRUE(waitUntil([&]() {
        return runtime.tryConsumeSnapshot(snapshot);
    }, std::chrono::milliseconds(2500)));
    EXPECT_FALSE(snapshot.empty());

    EXPECT_NE(runtime.statsAgeMs(), std::numeric_limits<std::uint32_t>::max());
    EXPECT_NE(runtime.snapshotAgeMs(), std::numeric_limits<std::uint32_t>::max());

    runtime.stop();
    backend.stop();
}

TEST(FrontendRuntimeContractRegression, StartFailsWhenRemoteBackendIsUnavailable)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;
    const std::uint16_t unavailablePort = backend.port();
    backend.stop();

    sim::FrontendRuntime runtime("simulation.ini", makeTransport(unavailablePort, ""));
    EXPECT_FALSE(runtime.start());
    EXPECT_EQ(runtime.linkStateLabel(), "reconnecting");
}

TEST(FrontendRuntimeContractRegression, ReconnectsWhenRealBackendRestarts)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;
    const std::uint16_t fixedPort = backend.port();

    sim::FrontendRuntime runtime("simulation.ini", makeTransport(fixedPort, backend.executablePath()));
    ASSERT_TRUE(runtime.start());

    ASSERT_TRUE(waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(4000)));

    backend.stop();
    ASSERT_TRUE(waitUntil([&]() {
        return runtime.linkStateLabel() == "reconnecting";
    }, std::chrono::milliseconds(4000)));

    ASSERT_TRUE(backend.start(startError, fixedPort)) << startError;
    runtime.requestReconnect();
    ASSERT_TRUE(waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(5000)));

    runtime.stop();
    backend.stop();
}

TEST(FrontendRuntimeContractRegression, ConnectorCanBeReconfiguredAtRuntimeToReachRealBackend)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    sim::FrontendRuntime runtime("simulation.ini", makeTransport(backend.port(), backend.executablePath()));
    ASSERT_TRUE(runtime.start());

    ASSERT_TRUE(waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(4000)));

    const std::uint16_t wrongPort = static_cast<std::uint16_t>(
        backend.port() == 65535u ? backend.port() - 1u : backend.port() + 1u);
    runtime.configureRemoteConnector("127.0.0.1", wrongPort, false, backend.executablePath());
    runtime.requestReconnect();
    ASSERT_TRUE(waitUntil([&]() {
        return runtime.linkStateLabel() == "reconnecting";
    }, std::chrono::milliseconds(3000)));

    runtime.configureRemoteConnector("127.0.0.1", backend.port(), false, backend.executablePath());
    runtime.requestReconnect();
    ASSERT_TRUE(waitUntil([&]() {
        return runtime.linkStateLabel() == "connected";
    }, std::chrono::milliseconds(5000)));

    EXPECT_TRUE(runtime.isRemoteMode());
    EXPECT_EQ(runtime.backendOwnerLabel(), "external");

    runtime.stop();
    backend.stop();
}

} // namespace
