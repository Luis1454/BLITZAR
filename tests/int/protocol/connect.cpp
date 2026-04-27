// File: tests/int/protocol/connect.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "protocol/ServerClient.hpp"
#include "protocol/ServerProtocol.hpp"
#include "tests/support/server_harness.hpp"
#include <chrono>
#include <cmath>
#include <gtest/gtest.h>
#include <thread>
#include <vector>
namespace grav_test_server_protocol_connect {
/// Description: Executes the TEST operation.
TEST(ServerProtocolTest, TST_INT_PROT_001_ServerClientParsesStatusAndSnapshotFromRealServer)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    ServerClient client;
    client.setSocketTimeoutMs(200);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(client.connect("127.0.0.1", server.port()));
    ServerClientStatus status{};
    const ServerClientResponse statusResponse = client.getStatus(status);
    ASSERT_TRUE(statusResponse.ok) << statusResponse.error;
    /// Description: Executes the EXPECT_GT operation.
    EXPECT_GT(status.particleCount, 0u);
    /// Description: Executes the EXPECT_GT operation.
    EXPECT_GT(status.dt, 0.0f);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(status.solver.empty());
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(status.integrator.empty());
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(status.faulted);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(status.faultReason.empty());
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(std::isfinite(status.totalEnergy));
    std::vector<RenderParticle> snapshot;
    ServerClientResponse snapshotResponse{};
    bool gotSnapshot = false;
    bool requestedStep = false;
    for (int attempt = 0; attempt < 200 && !gotSnapshot; ++attempt) {
        snapshotResponse = client.getSnapshot(snapshot, 128u);
        gotSnapshot = snapshotResponse.ok && !snapshot.empty();
        if (!gotSnapshot && !requestedStep && attempt >= 10) {
            const ServerClientResponse stepResponse =
                client.sendCommand(std::string(grav_protocol::Step), "\"count\":1");
            ASSERT_TRUE(stepResponse.ok) << stepResponse.error;
            requestedStep = true;
        }
        if (!gotSnapshot) {
            /// Description: Executes the sleep_for operation.
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
    }
    ASSERT_TRUE(gotSnapshot) << (snapshotResponse.error.empty() ? "snapshot stayed empty"
                                                                : snapshotResponse.error);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(snapshot.empty());
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(std::isfinite(snapshot.front().x));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(std::isfinite(snapshot.front().mass));
    client.disconnect();
    server.stop();
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolTest, TST_INT_PROT_002_ServerClientRecoversAfterRealServerRestartOnSamePort)
{
    RealServerHarness server;
    std::string startError;
    ASSERT_TRUE(server.start(startError)) << startError;
    const std::uint16_t fixedPort = server.port();
    ServerClient client;
    client.setSocketTimeoutMs(200);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(client.connect("127.0.0.1", fixedPort));
    ServerClientStatus status{};
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(client.getStatus(status).ok);
    server.stop();
    const ServerClientResponse afterStop = client.getStatus(status);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(afterStop.ok);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(afterStop.error.find("[server-client] sendJson:"), std::string::npos)
        << afterStop.error;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(client.isConnected());
    ASSERT_TRUE(server.start(startError, fixedPort)) << startError;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(client.connect("127.0.0.1", fixedPort));
    const ServerClientResponse afterRestart = client.getStatus(status);
    EXPECT_TRUE(afterRestart.ok) << afterRestart.error;
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(client.isConnected());
    client.disconnect();
    server.stop();
}
TEST(ServerProtocolTest,
     TST_INT_PROT_009_ServerClientRejectsRequestsWithoutConnectionWithOperationContext)
{
    ServerClient client;
    const ServerClientResponse response = client.sendJson("   ");
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(response.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(response.error, "[server-client] sendJson: not connected");
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolTest, TST_INT_PROT_003_ServerClientConnectTimeoutIsBounded)
{
    ServerClient client;
    client.setSocketTimeoutMs(120);
    const auto startedAt = std::chrono::steady_clock::now();
    const bool connected = client.connect("203.0.113.1", 65000u);
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        /// Description: Executes the now operation.
        std::chrono::steady_clock::now() - startedAt);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(connected);
    /// Description: Executes the EXPECT_LE operation.
    EXPECT_LE(elapsedMs.count(), 3000)
        << "connect timeout took too long: " << elapsedMs.count() << " ms";
}
} // namespace grav_test_server_protocol_connect
