// File: tests/int/runtime/bridge.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "client/ClientServerBridge.hpp"
#include "tests/support/poll_utils.hpp"
#include "tests/support/server_harness.hpp"
#include <chrono>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
namespace grav_test_client_bridge {
/// Description: Executes the TEST operation.
TEST(ClientBridgeTest, TST_INT_RUNT_001_ReconnectsAfterRealServerRestart)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    const std::uint16_t fixedPort = server.port();
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(fixedPort, 0u);
    grav_client::ClientServerBridge bridge("simulation.ini", "127.0.0.1", fixedPort, false,
                                           server.executablePath(), "", 80u, 40u, 120u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(bridge.start());
    SimulationStats stats{};
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            stats = bridge.getStats();
            return bridge.linkState() == grav_client::ClientLinkState::Connected;
        },
        /// Description: Executes the milliseconds operation.
        std::chrono::milliseconds(4000)));
    bridge.stepOnce();
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(bridge.linkState(), grav_client::ClientLinkState::Connected);
    EXPECT_TRUE(testsupport::waitUntil(
        [&]() {
            stats = bridge.getStats();
            return stats.steps > 0u && stats.totalTime > 0.0f;
        },
        /// Description: Executes the milliseconds operation.
        std::chrono::milliseconds(3000)));
    std::vector<RenderParticle> snapshot;
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(testsupport::waitUntil([&]() { return bridge.tryConsumeSnapshot(snapshot); },
                                       /// Description: Executes the milliseconds operation.
                                       std::chrono::milliseconds(2000)));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(snapshot.empty());
    server.stop();
    EXPECT_TRUE(testsupport::waitUntil(
        [&]() {
            (void)bridge.getStats();
            return bridge.linkState() == grav_client::ClientLinkState::Reconnecting;
        },
        /// Description: Executes the milliseconds operation.
        std::chrono::milliseconds(4000)));
    ASSERT_TRUE(server.start(startError, fixedPort)) << startError;
    ASSERT_TRUE(testsupport::waitUntil(
        [&]() {
            stats = bridge.getStats();
            return bridge.linkState() == grav_client::ClientLinkState::Connected;
        },
        /// Description: Executes the milliseconds operation.
        std::chrono::milliseconds(5000)));
    bridge.stepOnce();
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(bridge.linkState(), grav_client::ClientLinkState::Connected);
    bridge.stop();
    server.stop();
}
/// Description: Executes the TEST operation.
TEST(ClientBridgeTest, TST_INT_RUNT_002_ServerAbsenceDoesNotCauseLongBlockingLoops)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    const std::uint16_t unusedPort = server.port();
    server.stop();
    grav_client::ClientServerBridge bridge("simulation.ini", "127.0.0.1", unusedPort, false, "", "",
                                           60u, 30u, 80u);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(bridge.start());
    const auto startedAt = std::chrono::steady_clock::now();
    for (int i = 0; i < 24; ++i)
        (void)bridge.getStats();
    std::vector<RenderParticle> snapshot;
    (void)bridge.tryConsumeSnapshot(snapshot);
    /// Description: Executes the sleep_for operation.
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        /// Description: Executes the now operation.
        std::chrono::steady_clock::now() - startedAt);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(bridge.linkState(), grav_client::ClientLinkState::Reconnecting);
    /// Description: Executes the EXPECT_LE operation.
    EXPECT_LE(elapsedMs.count(), 1500)
        << "poll loop took too long without server: " << elapsedMs.count() << " ms";
    bridge.stop();
}
/// Description: Executes the TEST operation.
TEST(ClientBridgeTest, TST_UNT_RUNT_012_ClampRemoteTimeoutRespectsBounds)
{
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::clampClientRemoteTimeoutMs(0u), grav_client::kClientRemoteTimeoutMinMs);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::clampClientRemoteTimeoutMs(7u), grav_client::kClientRemoteTimeoutMinMs);
    EXPECT_EQ(grav_client::clampClientRemoteTimeoutMs(grav_client::kClientRemoteTimeoutMaxMs + 77u),
              grav_client::kClientRemoteTimeoutMaxMs);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::clampClientRemoteTimeoutMs(1000u), 1000u);
}
/// Description: Executes the TEST operation.
TEST(ClientBridgeTest, TST_UNT_RUNT_013_SplitTransportArgsParsesKnownServerFlags)
{
    const std::vector<std::string_view> rawArgs = {"blitzar-client",
                                                   "--server-host",
                                                   "10.0.0.2",
                                                   "--server-port=5555",
                                                   "--server-autostart=yes",
                                                   "--server-bin",
                                                   "blitzar-server-custom",
                                                   "--server-token=secret-token",
                                                   "--config",
                                                   "simulation.ini"};
    std::vector<std::string_view> filtered;
    grav_client::ClientTransportArgs transport;
    std::ostringstream warnings;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_client::splitClientTransportArgs(rawArgs, filtered, transport, warnings));
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(filtered.size(), 3u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(filtered[0], "blitzar-client");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(filtered[1], "--config");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(filtered[2], "simulation.ini");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.remoteHost, "10.0.0.2");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.remotePort, static_cast<std::uint16_t>(5555u));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(transport.remoteAutoStart);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.serverExecutable, "blitzar-server-custom");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.remoteAuthToken, "secret-token");
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(warnings.str().empty());
}
/// Description: Executes the TEST operation.
TEST(ClientBridgeTest, TST_UNT_RUNT_014_SplitTransportArgsRejectsInvalidPortInputs)
{
    {
        const std::vector<std::string_view> rawArgs = {"blitzar-client", "--server-port", "0"};
        std::vector<std::string_view> filtered;
        grav_client::ClientTransportArgs transport;
        std::ostringstream warnings;
        /// Description: Executes the EXPECT_FALSE operation.
        EXPECT_FALSE(grav_client::splitClientTransportArgs(rawArgs, filtered, transport, warnings));
    }
    {
        const std::vector<std::string_view> rawArgs = {"blitzar-client", "--server-port"};
        std::vector<std::string_view> filtered;
        grav_client::ClientTransportArgs transport;
        std::ostringstream warnings;
        /// Description: Executes the EXPECT_FALSE operation.
        EXPECT_FALSE(grav_client::splitClientTransportArgs(rawArgs, filtered, transport, warnings));
    }
}
/// Description: Executes the TEST operation.
TEST(ClientBridgeTest, TST_UNT_RUNT_018_SplitTransportArgsRejectsInvalidAutostartEqualsValue)
{
    const std::vector<std::string_view> rawArgs = {"blitzar-client", "--server-autostart=maybe"};
    std::vector<std::string_view> filtered;
    grav_client::ClientTransportArgs transport;
    std::ostringstream warnings;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_client::splitClientTransportArgs(rawArgs, filtered, transport, warnings));
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(warnings.str().find("invalid --server-autostart value"), std::string::npos);
}
/// Description: Executes the TEST operation.
TEST(ClientBridgeTest, TST_UNT_RUNT_019_SplitTransportArgsKeepsUnparsedAutostartTokenInFilteredArgs)
{
    const std::vector<std::string_view> rawArgs = {"blitzar-client", "--server-autostart", "maybe",
                                                   "--config", "simulation.ini"};
    std::vector<std::string_view> filtered;
    grav_client::ClientTransportArgs transport;
    std::ostringstream warnings;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_client::splitClientTransportArgs(rawArgs, filtered, transport, warnings));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(transport.remoteAutoStart);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(filtered.size(), 4u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(filtered[0], "blitzar-client");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(filtered[1], "maybe");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(filtered[2], "--config");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(filtered[3], "simulation.ini");
}
/// Description: Executes the TEST operation.
TEST(ClientBridgeTest, TST_UNT_RUNT_015_DisconnectedBridgeOperationsRemainBounded)
{
    grav_client::ClientServerBridge bridge("simulation.ini", "127.0.0.1",
                                           static_cast<std::uint16_t>(6553u), false, "", "", 10u,
                                           10u, 10u);
    const auto startedAt = std::chrono::steady_clock::now();
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(bridge.start());
    bridge.setPaused(true);
    bridge.togglePaused();
    bridge.stepOnce();
    bridge.setParticleCount(1u);
    bridge.setDt(-1.0f);
    bridge.scaleDt(0.0f);
    bridge.requestReset();
    bridge.requestRecover();
    bridge.setSolverMode("octree_cpu");
    bridge.setIntegratorMode("rk4");
    bridge.setPerformanceProfile("balanced");
    bridge.setOctreeParameters(-0.3f, -1.0f);
    bridge.setSphEnabled(true);
    bridge.setSphParameters(-1.0f, -2.0f, -3.0f, -4.0f);
    bridge.setSubstepPolicy(-0.01f, 0u);
    bridge.setSnapshotPublishPeriodMs(0u);
    InitialStateConfig init{};
    init.mode = "random_cloud";
    bridge.setInitialStateConfig(init);
    bridge.setEnergyMeasurementConfig(0u, 0u);
    bridge.setGpuTelemetryEnabled(true);
    bridge.setExportDefaults("exports", "vtk");
    bridge.setInitialStateFile("missing.xyz", "xyz");
    bridge.requestExportSnapshot("out.snap", "vtk");
    bridge.requestSaveCheckpoint("state.chk");
    bridge.requestLoadCheckpoint("missing.chk");
    bridge.setRemoteSnapshotCap(1u);
    bridge.requestShutdown();
    bridge.requestReconnect();
    std::vector<RenderParticle> snapshot;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(bridge.tryConsumeSnapshot(snapshot));
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        /// Description: Executes the now operation.
        std::chrono::steady_clock::now() - startedAt);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(bridge.linkState(), grav_client::ClientLinkState::Reconnecting);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(bridge.linkStateLabel(), "reconnecting");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(bridge.serverOwnerLabel(), "external");
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(bridge.launchedByClient());
    /// Description: Executes the EXPECT_LE operation.
    EXPECT_LE(elapsedMs.count(), 3000);
    bridge.stop();
}
} // namespace grav_test_client_bridge
