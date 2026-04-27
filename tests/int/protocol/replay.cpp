/*
 * @file tests/int/protocol/replay.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "protocol/ServerClient.hpp"
#include "protocol/ServerProtocol.hpp"
#include "tests/support/server_harness.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

namespace grav_test_server_protocol_replay {
static void waitForSteps(ServerClient& client, uint64_t targetSteps)
{
    ServerClientStatus status{};
    for (int attempt = 0; attempt < 100; ++attempt) {
        ServerClientResponse response = client.getStatus(status);
        ASSERT_TRUE(response.ok) << response.error;
        if (status.steps >= targetSteps) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    FAIL() << "Timeout waiting for simulation to reach step " << targetSteps;
}

TEST(ServerProtocolTest, TST_INT_PROT_011_FixedSeedServerReplayIsDeterministic)
{
    std::vector<std::string> args = {"--init-seed",      "424242", "--init-mode", "random_cloud",
                                     "--particle-count", "2000",   "--solver",    "octree_cpu",
                                     "--deterministic",  "true"};
    // Run Server A
    RealServerHarness serverA;
    std::string startErrorA;
    ASSERT_TRUE(serverA.start(startErrorA, 0u, "", args)) << startErrorA;
    ServerClient clientA;
    clientA.setSocketTimeoutMs(5000);
    ASSERT_TRUE(clientA.connect("127.0.0.1", serverA.port()));
    ServerClientResponse responseA = clientA.sendCommand(std::string(grav_protocol::Pause));
    ASSERT_TRUE(responseA.ok) << responseA.error;
    waitForSteps(clientA, 0u);
    std::vector<RenderParticle> snapshotZeroA;
    responseA = clientA.getSnapshot(snapshotZeroA, 4096u);
    ASSERT_TRUE(responseA.ok) << responseA.error;
    responseA = clientA.sendCommand(std::string(grav_protocol::Step), "\"count\":10");
    ASSERT_TRUE(responseA.ok) << responseA.error;
    waitForSteps(clientA, 10u);
    std::vector<RenderParticle> snapshotA;
    responseA = clientA.getSnapshot(snapshotA, 4096u);
    ASSERT_TRUE(responseA.ok) << responseA.error;
    clientA.disconnect();
    serverA.stop();
    // Verify snapshot is not stale (particles moved)
    ASSERT_EQ(snapshotZeroA.size(), snapshotA.size());
    ASSERT_GT(snapshotA.size(), 0u);
    bool particleMoved = false;
    for (size_t i = 0; i < snapshotA.size(); ++i) {
        if (snapshotZeroA[i].x != snapshotA[i].x || snapshotZeroA[i].y != snapshotA[i].y ||
            snapshotZeroA[i].z != snapshotA[i].z)
            particleMoved = true;
        break;
    }
    EXPECT_TRUE(particleMoved) << "Snapshot is stale: particles did not move after 10 steps";
    // Run Server B
    RealServerHarness serverB;
    std::string startErrorB;
    ASSERT_TRUE(serverB.start(startErrorB, 0u, "", args)) << startErrorB;
    ServerClient clientB;
    clientB.setSocketTimeoutMs(5000);
    ASSERT_TRUE(clientB.connect("127.0.0.1", serverB.port()));
    ServerClientResponse responseB =
        clientB.sendCommand(std::string(grav_protocol::Step), "\"count\":10");
    ASSERT_TRUE(responseB.ok) << responseB.error;
    waitForSteps(clientB, 10u);
    std::vector<RenderParticle> snapshotB;
    responseB = clientB.getSnapshot(snapshotB, 4096u);
    ASSERT_TRUE(responseB.ok) << responseB.error;
    clientB.disconnect();
    serverB.stop();
    // Verify determinism
    ASSERT_EQ(snapshotA.size(), snapshotB.size());
    for (size_t i = 0; i < snapshotA.size(); ++i) {
        EXPECT_EQ(snapshotA[i].x, snapshotB[i].x)
            << "Mismatch at particle " << i << " X coordinate (drift)";
        EXPECT_EQ(snapshotA[i].y, snapshotB[i].y)
            << "Mismatch at particle " << i << " Y coordinate (drift)";
        EXPECT_EQ(snapshotA[i].z, snapshotB[i].z)
            << "Mismatch at particle " << i << " Z coordinate (drift)";
    }
}
} // namespace grav_test_server_protocol_replay
