/*
 * @file tests/int/protocol/connect.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "protocol/client/Client.hpp"
#include "protocol/Protocol.hpp"
#include "tests/support/server_harness.hpp"
#include <chrono>
#include <cmath>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace bltzr_test_server_protocol_connect {
TEST(ServerProtocolTest, TST_INT_PROT_001_ClientParsesStatusAndSnapshotFromRealServer)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    bltzr_protocol::Client client;
    client.setSocketTimeoutMs(200);
    ASSERT_TRUE(client.connect("127.0.0.1", server.port()));
    bltzr_protocol::ClientStatus status{};
    const bltzr_protocol::Response statusResponse = client.getStatus(status);
    ASSERT_TRUE(statusResponse.ok) << statusResponse.error;
    EXPECT_GT(status.particleCount, 0u);
    EXPECT_GT(status.dt, 0.0f);
    EXPECT_FALSE(status.solver.empty());
    EXPECT_FALSE(status.integrator.empty());
    EXPECT_FALSE(status.faulted);
    EXPECT_TRUE(status.faultReason.empty());
    EXPECT_TRUE(std::isfinite(status.totalEnergy));
    std::vector<RenderParticle> snapshot;
    bltzr_protocol::Response snapshotResponse{};
    bool gotSnapshot = false;
    bool requestedStep = false;
    for (int attempt = 0; attempt < 200 && !gotSnapshot; ++attempt) {
        snapshotResponse = client.getSnapshot(snapshot, 128u);
        gotSnapshot = snapshotResponse.ok && !snapshot.empty();
        if (!gotSnapshot && !requestedStep && attempt >= 10) {
            const bltzr_protocol::Response stepResponse =
                client.sendCommand(std::string(bltzr_protocol::Step), "\"count\":1");
            ASSERT_TRUE(stepResponse.ok) << stepResponse.error;
            requestedStep = true;
        }
        if (!gotSnapshot) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
    }
    ASSERT_TRUE(gotSnapshot) << (snapshotResponse.error.empty() ? "snapshot stayed empty"
                                                                : snapshotResponse.error);
    ASSERT_FALSE(snapshot.empty());
    EXPECT_TRUE(std::isfinite(snapshot.front().x));
    EXPECT_TRUE(std::isfinite(snapshot.front().mass));
    client.disconnect();
    server.stop();
}

TEST(ServerProtocolTest, TST_INT_PROT_002_ClientRecoversAfterRealServerRestartOnSamePort)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    const std::uint16_t fixedPort = server.port();
    bltzr_protocol::Client client;
    client.setSocketTimeoutMs(200);
    ASSERT_TRUE(client.connect("127.0.0.1", fixedPort));
    bltzr_protocol::ClientStatus status{};
    ASSERT_TRUE(client.getStatus(status).ok);
    server.stop();
    const bltzr_protocol::Response afterStop = client.getStatus(status);
    EXPECT_FALSE(afterStop.ok);
    EXPECT_NE(afterStop.error.find("[server-client] sendJson:"), std::string::npos)
        << afterStop.error;
    EXPECT_FALSE(client.isConnected());
    ASSERT_TRUE(server.start(startError, fixedPort)) << startError;
    ASSERT_TRUE(client.connect("127.0.0.1", fixedPort));
    const bltzr_protocol::Response afterRestart = client.getStatus(status);
    EXPECT_TRUE(afterRestart.ok) << afterRestart.error;
    EXPECT_TRUE(client.isConnected());
    client.disconnect();
    server.stop();
}

TEST(ServerProtocolTest,
     TST_INT_PROT_009_ClientRejectsRequestsWithoutConnectionWithOperationContext)
{
    bltzr_protocol::Client client;
    const bltzr_protocol::Response response = client.sendJson("   ");
    EXPECT_FALSE(response.ok);
    EXPECT_EQ(response.error, "[server-client] sendJson: not connected");
}

TEST(ServerProtocolTest, TST_INT_PROT_003_ClientConnectTimeoutIsBounded)
{
    bltzr_protocol::Client client;
    client.setSocketTimeoutMs(120);
    const auto startedAt = std::chrono::steady_clock::now();
    const bool connected = client.connect("203.0.113.1", 65000u);
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startedAt);
    EXPECT_FALSE(connected);
    EXPECT_LE(elapsedMs.count(), 3000)
        << "connect timeout took too long: " << elapsedMs.count() << " ms";
}
} // namespace bltzr_test_server_protocol_connect
