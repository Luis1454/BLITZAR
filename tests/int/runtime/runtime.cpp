/*
 * @file tests/int/runtime/runtime.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "client/ClientRuntime.hpp"
#include "tests/support/client_utils.hpp"
#include "tests/support/poll_utils.hpp"
#include "tests/support/server_harness.hpp"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace bltzr_test_client_runtime {
static std::filesystem::path writeSnapshotPipelineConfig(const char* basename,
                                                         std::uint32_t queueCapacity,
                                                         const char* dropPolicy)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        (std::string(basename) + "_" + std::to_string(stamp) + ".ini");
    std::ofstream out(path, std::ios::trunc);
    out << "client_snapshot_queue_capacity=" << queueCapacity << "\n";
    out << "client_snapshot_drop_policy=" << dropPolicy << "\n";
    return path;
}

TEST(ClientRuntimeTest, TST_CNT_RUNT_001_ConnectsToRealServerAndPublishesStatsAndSnapshot)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    bltzr_client::ClientRuntime runtime(
        "simulation.ini", testsupport::makeTransport(server.port(), server.executablePath()));
    ASSERT_TRUE(runtime.start());
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            return runtime.linkStateLabel() == "connected";
        },
        std::chrono::milliseconds(4000)));
    const SimulationStats stats = runtime.getStats();
    EXPECT_GT(stats.particleCount, 0u);
    EXPECT_GT(stats.dt, 0.0f);
    EXPECT_FALSE(stats.solverName.empty());
    EXPECT_FALSE(stats.faulted);
    std::optional<bltzr_client::ConsumedSnapshot> consumedSnapshot;
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            consumedSnapshot = runtime.consumeLatestSnapshot();
            return consumedSnapshot.has_value();
        },
        std::chrono::milliseconds(2500)));
    ASSERT_TRUE(consumedSnapshot.has_value());
    std::vector<RenderParticle> snapshot = std::move(consumedSnapshot->particles);
    EXPECT_FALSE(snapshot.empty());
    EXPECT_EQ(consumedSnapshot->sourceSize, snapshot.size());
    EXPECT_NE(consumedSnapshot->latencyMs, std::numeric_limits<std::uint32_t>::max());
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
    bltzr_client::ClientRuntime runtime("simulation.ini",
                                        testsupport::makeTransport(unavailablePort, ""));
    EXPECT_FALSE(runtime.start());
    EXPECT_EQ(runtime.linkStateLabel(), "reconnecting");
}

TEST(ClientRuntimeTest, TST_CNT_RUNT_003_ReconnectsWhenRealServerRestarts)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    const std::uint16_t fixedPort = server.port();
    bltzr_client::ClientRuntime runtime(
        "simulation.ini", testsupport::makeTransport(fixedPort, server.executablePath()));
    ASSERT_TRUE(runtime.start());
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            return runtime.linkStateLabel() == "connected";
        },
        std::chrono::milliseconds(4000)));
    server.stop();
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            return runtime.linkStateLabel() == "reconnecting";
        },
        std::chrono::milliseconds(4000)));
    ASSERT_TRUE(server.start(startError, fixedPort)) << startError;
    runtime.requestReconnect();
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            return runtime.linkStateLabel() == "connected";
        },
        std::chrono::milliseconds(5000)));
    runtime.stop();
    server.stop();
}

TEST(ClientRuntimeTest, TST_CNT_RUNT_004_ConnectorCanBeReconfiguredAtRuntimeToReachRealServer)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    bltzr_client::ClientRuntime runtime(
        "simulation.ini", testsupport::makeTransport(server.port(), server.executablePath()));
    ASSERT_TRUE(runtime.start());
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            return runtime.linkStateLabel() == "connected";
        },
        std::chrono::milliseconds(4000)));
    const std::uint16_t wrongPort = static_cast<std::uint16_t>(
        server.port() == 65535u ? server.port() - 1u : server.port() + 1u);
    runtime.configureRemoteConnector("127.0.0.1", wrongPort, false, server.executablePath());
    runtime.requestReconnect();
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            return runtime.linkStateLabel() == "reconnecting";
        },
        std::chrono::milliseconds(3000)));
    runtime.configureRemoteConnector("127.0.0.1", server.port(), false, server.executablePath());
    runtime.requestReconnect();
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            return runtime.linkStateLabel() == "connected";
        },
        std::chrono::milliseconds(5000)));
    EXPECT_TRUE(runtime.isRemoteMode());
    EXPECT_EQ(runtime.serverOwnerLabel(), "external");
    runtime.stop();
    server.stop();
}

TEST(ClientRuntimeTest, TST_CNT_RUNT_009_LatestOnlySnapshotQueueKeepsLatencyLowWhileDroppingBacklog)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    const std::filesystem::path configPath =
        writeSnapshotPipelineConfig("BLITZAR_runtime_latest_only", 2u, "latest-only");
    bltzr_client::ClientRuntime runtime(
        configPath.string(), testsupport::makeTransport(server.port(), server.executablePath()));
    ASSERT_TRUE(runtime.start());
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            const bltzr_client::SnapshotPipelineState state = runtime.snapshotPipelineState();
            return runtime.linkStateLabel() == "connected" && state.queueDepth == 2u &&
                   state.droppedFrames > 0u;
        },
        std::chrono::milliseconds(5000)));
    const bltzr_client::SnapshotPipelineState state = runtime.snapshotPipelineState();
    EXPECT_EQ(state.dropPolicy, "latest-only");
    EXPECT_EQ(state.queueCapacity, 2u);
    EXPECT_EQ(state.queueDepth, 2u);
    EXPECT_GT(state.droppedFrames, 0u);
    std::optional<bltzr_client::ConsumedSnapshot> consumedSnapshot =
        runtime.consumeLatestSnapshot();
    ASSERT_TRUE(consumedSnapshot.has_value());
    EXPECT_LT(consumedSnapshot->latencyMs, 250u);
    runtime.stop();
    server.stop();
    std::error_code ec;
    std::filesystem::remove(configPath, ec);
}

TEST(ClientRuntimeTest, TST_CNT_RUNT_010_PacedSnapshotQueuePreservesBacklogAndReportsLatencyGrowth)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    const std::filesystem::path configPath =
        writeSnapshotPipelineConfig("BLITZAR_runtime_paced", 2u, "paced");
    bltzr_client::ClientRuntime runtime(
        configPath.string(), testsupport::makeTransport(server.port(), server.executablePath()));
    ASSERT_TRUE(runtime.start());
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            const bltzr_client::SnapshotPipelineState state = runtime.snapshotPipelineState();
            return runtime.linkStateLabel() == "connected" && state.queueDepth == 2u &&
                   state.droppedFrames > 0u &&
                   state.latencyMs != std::numeric_limits<std::uint32_t>::max() &&
                   state.latencyMs >= 250u;
        },
        std::chrono::milliseconds(5000)));
    const bltzr_client::SnapshotPipelineState state = runtime.snapshotPipelineState();
    EXPECT_EQ(state.dropPolicy, "paced");
    EXPECT_EQ(state.queueCapacity, 2u);
    EXPECT_EQ(state.queueDepth, 2u);
    EXPECT_GT(state.droppedFrames, 0u);
    EXPECT_GE(state.latencyMs, 250u);
    std::optional<bltzr_client::ConsumedSnapshot> consumedSnapshot =
        runtime.consumeLatestSnapshot();
    ASSERT_TRUE(consumedSnapshot.has_value());
    EXPECT_GE(consumedSnapshot->latencyMs, 250u);
    runtime.stop();
    server.stop();
    std::error_code ec;
    std::filesystem::remove(configPath, ec);
}

TEST(ClientRuntimeTest, TST_CNT_RUNT_011_BackendSnapshotPolicyReducesTransferForLowDrawCap)
{
    RealServerHarness server;
    std::string startError;
    const std::vector<std::string> serverArgs{
        "--particle-count",    "40000",        "--solver",
        "octree_cpu",          "--integrator", "euler",
        "--init-config-style", "preset",       "--preset-structure",
        "random_cloud"};
    ASSERT_TRUE(server.start(startError, 0u, {}, serverArgs)) << startError;
    bltzr_client::ClientRuntime runtime(
        "simulation.ini", testsupport::makeTransport(server.port(), server.executablePath()));
    ASSERT_TRUE(runtime.start());
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            const SimulationStats stats = runtime.getStats();
            return runtime.linkStateLabel() == "connected" && stats.particleCount == 40000u &&
                   stats.solverName == "octree_cpu";
        },
        std::chrono::milliseconds(4000)));
    runtime.setRemoteSnapshotCap(4096u);
    std::optional<bltzr_client::ConsumedSnapshot> consumedSnapshot;
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            consumedSnapshot = runtime.consumeLatestSnapshot();
            return consumedSnapshot.has_value() && consumedSnapshot->sourceSize == 8192u;
        },
        std::chrono::milliseconds(4000)));
    ASSERT_TRUE(consumedSnapshot.has_value());
    EXPECT_EQ(runtime.getStats().particleCount, 40000u);
    EXPECT_EQ(consumedSnapshot->particles.size(), 4096u);
    EXPECT_LT(consumedSnapshot->sourceSize, 40000u);
    EXPECT_EQ(consumedSnapshot->sourceSize, 8192u);
    runtime.stop();
    server.stop();
}
} // namespace bltzr_test_client_runtime
