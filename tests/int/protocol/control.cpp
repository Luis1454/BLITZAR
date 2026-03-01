#include "protocol/BackendClient.hpp"
#include "protocol/BackendProtocol.hpp"
#include "tests/support/backend_harness.hpp"
#include "tests/support/scoped_env_var.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>

namespace {

TEST(BackendProtocolTest, TST_INT_PROT_004_BackendAcceptsControlCommandsFromClient)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    BackendClient client;
    client.setSocketTimeoutMs(200);
    ASSERT_TRUE(client.connect("127.0.0.1", backend.port()));

    BackendClientResponse response = client.sendCommand(std::string(grav_protocol::SetDt), "\"value\":0.02");
    ASSERT_TRUE(response.ok) << response.error;

    response = client.sendCommand(std::string(grav_protocol::SetSolver), "\"value\":\"pairwise_cuda\"");
    ASSERT_TRUE(response.ok) << response.error;

    response = client.sendCommand(std::string(grav_protocol::Step), "\"count\":1");
    ASSERT_TRUE(response.ok) << response.error;

    response = client.sendCommand(std::string(grav_protocol::Recover));
    ASSERT_TRUE(response.ok) << response.error;

    BackendClientStatus status{};
    ASSERT_TRUE(client.getStatus(status).ok);
    EXPECT_NEAR(status.dt, 0.02f, 1e-6f);
    EXPECT_EQ(status.solver, "pairwise_cuda");
    EXPECT_FALSE(status.faulted);

    client.disconnect();
    backend.stop();
}

TEST(BackendProtocolTest, TST_INT_PROT_005_BackendRejectsInvalidSolverAndIntegratorCommands)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    BackendClient client;
    client.setSocketTimeoutMs(200);
    ASSERT_TRUE(client.connect("127.0.0.1", backend.port()));

    BackendClientResponse response = client.sendCommand(std::string(grav_protocol::SetSolver), "\"value\":\"not_a_solver\"");
    ASSERT_FALSE(response.ok);
    EXPECT_NE(response.error.find("invalid solver"), std::string::npos);

    response = client.sendCommand(std::string(grav_protocol::SetIntegrator), "\"value\":\"not_an_integrator\"");
    ASSERT_FALSE(response.ok);
    EXPECT_NE(response.error.find("invalid integrator"), std::string::npos);

    client.disconnect();
    backend.stop();
}

TEST(BackendProtocolTest, TST_INT_PROT_006_BackendCoercesUnsupportedIntegratorForOctreeGpu)
{
    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    BackendClient client;
    client.setSocketTimeoutMs(200);
    ASSERT_TRUE(client.connect("127.0.0.1", backend.port()));

    BackendClientResponse response = client.sendCommand(std::string(grav_protocol::Pause));
    ASSERT_TRUE(response.ok) << response.error;

    response = client.sendCommand(std::string(grav_protocol::SetSolver), "\"value\":\"octree_gpu\"");
    ASSERT_TRUE(response.ok) << response.error;

    response = client.sendCommand(std::string(grav_protocol::SetIntegrator), "\"value\":\"rk4\"");
    ASSERT_TRUE(response.ok) << response.error;

    BackendClientStatus status{};
    ASSERT_TRUE(client.getStatus(status).ok);
    EXPECT_EQ(status.solver, "octree_gpu");
    EXPECT_EQ(status.integrator, "euler");

    client.disconnect();
    backend.stop();
}

TEST(BackendProtocolTest, TST_INT_PROT_007_BackendFallsBackToCpuAfterForcedCudaFailure)
{
    testsupport::ScopedEnvVar forceCudaFail("GRAVITY_TEST_FORCE_CUDA_FAIL_ONCE", "1");

    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    BackendClient client;
    client.setSocketTimeoutMs(200);
    ASSERT_TRUE(client.connect("127.0.0.1", backend.port()));

    BackendClientResponse response = client.sendCommand(std::string(grav_protocol::SetSolver), "\"value\":\"octree_gpu\"");
    ASSERT_TRUE(response.ok) << response.error;
    response = client.sendCommand(std::string(grav_protocol::SetIntegrator), "\"value\":\"euler\"");
    ASSERT_TRUE(response.ok) << response.error;

    response = client.sendCommand(std::string(grav_protocol::Step), "\"count\":1");
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

TEST(BackendProtocolTest, TST_INT_PROT_008_BackendRejectsRequestsWithoutConfiguredToken)
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
