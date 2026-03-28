#include "client/ClientServerBridge.hpp"
#include "tests/support/poll_utils.hpp"
#include "tests/support/server_harness.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <limits>
#include <sstream>
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
    EXPECT_TRUE(testsupport::waitUntil([&]() {
        stats = bridge.getStats();
        return stats.steps > 0u && stats.totalTime > 0.0f;
    }, std::chrono::milliseconds(3000)));

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

TEST(ClientBridgeTest, TST_UNT_RUNT_012_ClampRemoteTimeoutRespectsBounds)
{
    EXPECT_EQ(grav_client::clampClientRemoteTimeoutMs(0u), grav_client::kClientRemoteTimeoutMinMs);
    EXPECT_EQ(grav_client::clampClientRemoteTimeoutMs(7u), grav_client::kClientRemoteTimeoutMinMs);
    EXPECT_EQ(
        grav_client::clampClientRemoteTimeoutMs(grav_client::kClientRemoteTimeoutMaxMs + 77u),
        grav_client::kClientRemoteTimeoutMaxMs);
    EXPECT_EQ(grav_client::clampClientRemoteTimeoutMs(1000u), 1000u);
}

TEST(ClientBridgeTest, TST_UNT_RUNT_013_SplitTransportArgsParsesKnownServerFlags)
{
    const std::vector<std::string_view> rawArgs = {
        "blitzar-client",
        "--server-host",
        "10.0.0.2",
        "--server-port=5555",
        "--server-autostart=yes",
        "--server-bin",
        "blitzar-server-custom",
        "--server-token=secret-token",
        "--config",
        "simulation.ini"
    };

    std::vector<std::string_view> filtered;
    grav_client::ClientTransportArgs transport;
    std::ostringstream warnings;

    ASSERT_TRUE(grav_client::splitClientTransportArgs(rawArgs, filtered, transport, warnings));
    ASSERT_EQ(filtered.size(), 3u);
    EXPECT_EQ(filtered[0], "blitzar-client");
    EXPECT_EQ(filtered[1], "--config");
    EXPECT_EQ(filtered[2], "simulation.ini");

    EXPECT_EQ(transport.remoteHost, "10.0.0.2");
    EXPECT_EQ(transport.remotePort, static_cast<std::uint16_t>(5555u));
    EXPECT_TRUE(transport.remoteAutoStart);
    EXPECT_EQ(transport.serverExecutable, "blitzar-server-custom");
    EXPECT_EQ(transport.remoteAuthToken, "secret-token");
    EXPECT_TRUE(warnings.str().empty());
}

TEST(ClientBridgeTest, TST_UNT_RUNT_014_SplitTransportArgsRejectsInvalidPortInputs)
{
    {
        const std::vector<std::string_view> rawArgs = {"blitzar-client", "--server-port", "0"};
        std::vector<std::string_view> filtered;
        grav_client::ClientTransportArgs transport;
        std::ostringstream warnings;
        EXPECT_FALSE(grav_client::splitClientTransportArgs(rawArgs, filtered, transport, warnings));
    }
    {
        const std::vector<std::string_view> rawArgs = {"blitzar-client", "--server-port"};
        std::vector<std::string_view> filtered;
        grav_client::ClientTransportArgs transport;
        std::ostringstream warnings;
        EXPECT_FALSE(grav_client::splitClientTransportArgs(rawArgs, filtered, transport, warnings));
    }
}

TEST(ClientBridgeTest, TST_UNT_RUNT_018_SplitTransportArgsRejectsInvalidAutostartEqualsValue)
{
    const std::vector<std::string_view> rawArgs = {
        "blitzar-client",
        "--server-autostart=maybe"
    };

    std::vector<std::string_view> filtered;
    grav_client::ClientTransportArgs transport;
    std::ostringstream warnings;

    EXPECT_FALSE(grav_client::splitClientTransportArgs(rawArgs, filtered, transport, warnings));
    EXPECT_NE(warnings.str().find("invalid --server-autostart value"), std::string::npos);
}

TEST(ClientBridgeTest, TST_UNT_RUNT_019_SplitTransportArgsKeepsUnparsedAutostartTokenInFilteredArgs)
{
    const std::vector<std::string_view> rawArgs = {
        "blitzar-client",
        "--server-autostart",
        "maybe",
        "--config",
        "simulation.ini"
    };

    std::vector<std::string_view> filtered;
    grav_client::ClientTransportArgs transport;
    std::ostringstream warnings;

    ASSERT_TRUE(grav_client::splitClientTransportArgs(rawArgs, filtered, transport, warnings));
    EXPECT_FALSE(transport.remoteAutoStart);
    ASSERT_EQ(filtered.size(), 4u);
    EXPECT_EQ(filtered[0], "blitzar-client");
    EXPECT_EQ(filtered[1], "maybe");
    EXPECT_EQ(filtered[2], "--config");
    EXPECT_EQ(filtered[3], "simulation.ini");
}

TEST(ClientBridgeTest, TST_UNT_RUNT_015_DisconnectedBridgeOperationsRemainBounded)
{
    grav_client::ClientServerBridge bridge(
        "simulation.ini",
        "127.0.0.1",
        static_cast<std::uint16_t>(6553u),
        false,
        "",
        "",
        10u,
        10u,
        10u);

    const auto startedAt = std::chrono::steady_clock::now();
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
    EXPECT_FALSE(bridge.tryConsumeSnapshot(snapshot));
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startedAt);

    EXPECT_EQ(bridge.linkState(), grav_client::ClientLinkState::Reconnecting);
    EXPECT_EQ(bridge.linkStateLabel(), "reconnecting");
    EXPECT_EQ(bridge.serverOwnerLabel(), "external");
    EXPECT_FALSE(bridge.launchedByClient());
    EXPECT_LE(elapsedMs.count(), 3000);

    bridge.stop();
}

} // namespace grav_test_client_bridge



