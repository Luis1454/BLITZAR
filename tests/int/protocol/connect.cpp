#include "protocol/BackendClient.hpp"
#include "protocol/BackendProtocol.hpp"
#include "tests/support/backend_harness.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

namespace grav_test_backend_protocol_connect {

TEST(BackendProtocolTest, TST_INT_PROT_001_BackendClientParsesStatusAndSnapshotFromRealBackend)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    BackendClient client;
    client.setSocketTimeoutMs(200);
    ASSERT_TRUE(client.connect("127.0.0.1", backend.port()));

    BackendClientStatus status{};
    const BackendClientResponse statusResponse = client.getStatus(status);
    ASSERT_TRUE(statusResponse.ok) << statusResponse.error;
    EXPECT_GT(status.particleCount, 0u);
    EXPECT_GT(status.dt, 0.0f);
    EXPECT_FALSE(status.solver.empty());
    EXPECT_FALSE(status.integrator.empty());
    EXPECT_FALSE(status.faulted);
    EXPECT_TRUE(status.faultReason.empty());
    EXPECT_TRUE(std::isfinite(status.totalEnergy));

    std::vector<RenderParticle> snapshot;
    BackendClientResponse snapshotResponse{};
    bool gotSnapshot = false;
    bool requestedStep = false;
    for (int atmp = 0; atmp < 200 && !gotSnapshot; ++atmp) {
        snapshotResponse = client.getSnapshot(snapshot, 128u);
        gotSnapshot = snapshotResponse.ok && !snapshot.empty();
        if (!gotSnapshot && !requestedStep && atmp >= 10) {
            const BackendClientResponse stepResponse = client.sendCommand(
                std::string(grav_protocol::Step),
                "\"count\":1");
            ASSERT_TRUE(stepResponse.ok) << stepResponse.error;
            requestedStep = true;
        }
        if (!gotSnapshot) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
    }
    ASSERT_TRUE(gotSnapshot) << (snapshotResponse.error.empty() ? "snapshot stayed empty" : snapshotResponse.error);
    ASSERT_FALSE(snapshot.empty());
    EXPECT_TRUE(std::isfinite(snapshot.front().x));
    EXPECT_TRUE(std::isfinite(snapshot.front().mass));

    client.disconnect();
    backend.stop();
}

TEST(BackendProtocolTest, TST_INT_PROT_002_BackendClientRecoversAfterRealBackendRestartOnSamePort)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;
    const std::uint16_t fixedPort = backend.port();

    BackendClient client;
    client.setSocketTimeoutMs(200);
    ASSERT_TRUE(client.connect("127.0.0.1", fixedPort));
    BackendClientStatus status{};
    ASSERT_TRUE(client.getStatus(status).ok);

    backend.stop();
    const BackendClientResponse afterStop = client.getStatus(status);
    EXPECT_FALSE(afterStop.ok);
    EXPECT_FALSE(client.isConnected());

    ASSERT_TRUE(backend.start(startError, fixedPort)) << startError;
    ASSERT_TRUE(client.connect("127.0.0.1", fixedPort));
    const BackendClientResponse afterRestart = client.getStatus(status);
    EXPECT_TRUE(afterRestart.ok) << afterRestart.error;
    EXPECT_TRUE(client.isConnected());

    client.disconnect();
    backend.stop();
}

TEST(BackendProtocolTest, TST_INT_PROT_003_BackendClientConnectTimeoutIsBounded)
{
    BackendClient client;
    client.setSocketTimeoutMs(120);

    const auto startedAt = std::chrono::steady_clock::now();
    const bool connected = client.connect("203.0.113.1", 65000u);
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startedAt);

    EXPECT_FALSE(connected);
    EXPECT_LE(elapsedMs.count(), 3000) << "connect timeout took too long: " << elapsedMs.count() << " ms";
}

} // namespace grav_test_backend_protocol_connect


