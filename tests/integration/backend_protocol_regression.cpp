#include "sim/BackendClient.hpp"
#include "sim/BackendProtocol.hpp"
#include "tests/integration/RealBackendHarness.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace {

class ScopedEnvVar {
    public:
        ScopedEnvVar(const char *name, const char *value)
            : _name(name), _hadValue(false)
        {
            const char *current = std::getenv(name);
            if (current != nullptr) {
                _hadValue = true;
                _previousValue = current;
            }
            set(value);
        }

        ~ScopedEnvVar()
        {
            if (_hadValue) {
                set(_previousValue.c_str());
            } else {
#if defined(_WIN32)
                _putenv_s(_name.c_str(), "");
#else
                unsetenv(_name.c_str());
#endif
            }
        }

        ScopedEnvVar(const ScopedEnvVar &) = delete;
        ScopedEnvVar &operator=(const ScopedEnvVar &) = delete;

    private:
        void set(const char *value)
        {
#if defined(_WIN32)
            _putenv_s(_name.c_str(), value);
#else
            setenv(_name.c_str(), value, 1);
#endif
        }

        std::string _name;
        std::string _previousValue;
        bool _hadValue;
};

TEST(BackendProtocolRegression, BackendClientParsesStatusAndSnapshotFromRealBackend)
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
    for (int attempt = 0; attempt < 20 && !gotSnapshot; ++attempt) {
        snapshotResponse = client.getSnapshot(snapshot, 128u);
        gotSnapshot = snapshotResponse.ok && !snapshot.empty();
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

TEST(BackendProtocolRegression, BackendClientRecoversAfterRealBackendRestartOnSamePort)
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

TEST(BackendProtocolRegression, BackendClientConnectTimeoutIsBounded)
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

TEST(BackendProtocolRegression, BackendAcceptsControlCommandsFromClient)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    BackendClient client;
    client.setSocketTimeoutMs(200);
    ASSERT_TRUE(client.connect("127.0.0.1", backend.port()));

    BackendClientResponse response = client.sendCommand(std::string(sim::protocol::cmd::SetDt), "\"value\":0.02");
    ASSERT_TRUE(response.ok) << response.error;

    response = client.sendCommand(std::string(sim::protocol::cmd::SetSolver), "\"value\":\"pairwise_cuda\"");
    ASSERT_TRUE(response.ok) << response.error;

    response = client.sendCommand(std::string(sim::protocol::cmd::Step), "\"count\":1");
    ASSERT_TRUE(response.ok) << response.error;

    response = client.sendCommand(std::string(sim::protocol::cmd::Recover));
    ASSERT_TRUE(response.ok) << response.error;

    BackendClientStatus status{};
    ASSERT_TRUE(client.getStatus(status).ok);
    EXPECT_NEAR(status.dt, 0.02f, 1e-6f);
    EXPECT_EQ(status.solver, "pairwise_cuda");
    EXPECT_FALSE(status.faulted);

    client.disconnect();
    backend.stop();
}

TEST(BackendProtocolRegression, BackendRejectsInvalidSolverAndIntegratorCommands)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    BackendClient client;
    client.setSocketTimeoutMs(200);
    ASSERT_TRUE(client.connect("127.0.0.1", backend.port()));

    BackendClientResponse response = client.sendCommand(std::string(sim::protocol::cmd::SetSolver), "\"value\":\"not_a_solver\"");
    ASSERT_FALSE(response.ok);
    EXPECT_NE(response.error.find("invalid solver"), std::string::npos);

    response = client.sendCommand(std::string(sim::protocol::cmd::SetIntegrator), "\"value\":\"not_an_integrator\"");
    ASSERT_FALSE(response.ok);
    EXPECT_NE(response.error.find("invalid integrator"), std::string::npos);

    client.disconnect();
    backend.stop();
}

TEST(BackendProtocolRegression, BackendCoercesUnsupportedIntegratorForOctreeGpu)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    BackendClient client;
    client.setSocketTimeoutMs(200);
    ASSERT_TRUE(client.connect("127.0.0.1", backend.port()));

    BackendClientResponse response = client.sendCommand(std::string(sim::protocol::cmd::Pause));
    ASSERT_TRUE(response.ok) << response.error;

    response = client.sendCommand(std::string(sim::protocol::cmd::SetSolver), "\"value\":\"octree_gpu\"");
    ASSERT_TRUE(response.ok) << response.error;

    response = client.sendCommand(std::string(sim::protocol::cmd::SetIntegrator), "\"value\":\"rk4\"");
    ASSERT_TRUE(response.ok) << response.error;

    BackendClientStatus status{};
    ASSERT_TRUE(client.getStatus(status).ok);
    EXPECT_EQ(status.solver, "octree_gpu");
    EXPECT_EQ(status.integrator, "euler");

    client.disconnect();
    backend.stop();
}

TEST(BackendProtocolRegression, BackendFallsBackToCpuAfterForcedCudaFailure)
{
    ScopedEnvVar forceCudaFail("GRAVITY_TEST_FORCE_CUDA_FAIL_ONCE", "1");

    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    BackendClient client;
    client.setSocketTimeoutMs(200);
    ASSERT_TRUE(client.connect("127.0.0.1", backend.port()));

    BackendClientResponse response = client.sendCommand(std::string(sim::protocol::cmd::SetSolver), "\"value\":\"octree_gpu\"");
    ASSERT_TRUE(response.ok) << response.error;
    response = client.sendCommand(std::string(sim::protocol::cmd::SetIntegrator), "\"value\":\"euler\"");
    ASSERT_TRUE(response.ok) << response.error;

    response = client.sendCommand(std::string(sim::protocol::cmd::Step), "\"count\":1");
    ASSERT_TRUE(response.ok) << response.error;

    BackendClientStatus status{};
    bool switchedToCpu = false;
    for (int attempt = 0; attempt < 60; ++attempt) {
        const BackendClientResponse statusResponse = client.getStatus(status);
        ASSERT_TRUE(statusResponse.ok) << statusResponse.error;
        if (status.solver == "octree_cpu") {
            switchedToCpu = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ASSERT_TRUE(switchedToCpu) << "solver never switched to octree_cpu after forced CUDA failure";
    EXPECT_FALSE(status.faulted);
    EXPECT_TRUE(status.faultReason.empty());

    client.disconnect();
    backend.stop();
}

TEST(BackendProtocolRegression, BackendRejectsRequestsWithoutConfiguredToken)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError, 0u, "secret-token")) << startError;

    BackendClient client;
    client.setSocketTimeoutMs(200);
    ASSERT_TRUE(client.connect("127.0.0.1", backend.port()));

    BackendClientStatus status{};
    BackendClientResponse response = client.getStatus(status);
    ASSERT_FALSE(response.ok);
    EXPECT_NE(response.error.find("unauthorized"), std::string::npos);
    client.disconnect();

    client.setAuthToken("secret-token");
    ASSERT_TRUE(client.connect("127.0.0.1", backend.port()));
    response = client.getStatus(status);
    ASSERT_TRUE(response.ok) << response.error;
    EXPECT_GT(status.particleCount, 0u);

    client.disconnect();
    backend.stop();
}

} // namespace
