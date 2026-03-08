#include "protocol/BackendJsonCodec.hpp"
#include "protocol/BackendProtocol.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace grav_test_protocol_codec {

TEST(BackendProtocolCodecTest, TST_UNT_PROT_001_ParsesCommandEnvelopeWithEscapedToken)
{
    grav_protocol::BackendCommandRequest request{};
    request.cmd = std::string(grav_protocol::SetSolver);
    request.token = "secret\"token";

    const std::string json = grav_protocol::BackendJsonCodec::makeCommandRequest(
        request,
        "\"value\":\"octree_gpu\"");

    grav_protocol::BackendCommandRequest parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::BackendJsonCodec::parseCommandRequest(json, parsed, error)) << error;
    EXPECT_EQ(parsed.cmd, grav_protocol::SetSolver);
    EXPECT_EQ(parsed.token, "secret\"token");
}

TEST(BackendProtocolCodecTest, TST_UNT_PROT_002_ParsesTypedStatusPayload)
{
    SimulationStats stats{};
    stats.steps = 42u;
    stats.dt = 0.02f;
    stats.paused = true;
    stats.faulted = true;
    stats.faultStep = 41u;
    stats.faultReason = "bad\\nstate";
    stats.sphEnabled = true;
    stats.backendFps = 144.5f;
    stats.particleCount = 128u;
    stats.solverName = "octree_gpu";
    stats.integratorName = "euler";
    stats.kineticEnergy = 1.0f;
    stats.potentialEnergy = -2.0f;
    stats.thermalEnergy = 3.0f;
    stats.radiatedEnergy = 4.0f;
    stats.totalEnergy = 5.0f;
    stats.energyDriftPct = 0.125f;
    stats.energyEstimated = true;

    const std::string raw = grav_protocol::BackendJsonCodec::makeStatusResponse(stats);

    grav_protocol::BackendStatusPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::BackendJsonCodec::parseStatusResponse(raw, parsed, error)) << error;
    ASSERT_TRUE(parsed.envelope.ok);
    EXPECT_EQ(parsed.steps, 42u);
    EXPECT_FLOAT_EQ(parsed.dt, 0.02f);
    EXPECT_TRUE(parsed.paused);
    EXPECT_TRUE(parsed.faulted);
    EXPECT_EQ(parsed.faultStep, 41u);
    EXPECT_EQ(parsed.faultReason, "bad\\nstate");
    EXPECT_TRUE(parsed.sphEnabled);
    EXPECT_FLOAT_EQ(parsed.backendFps, 144.5f);
    EXPECT_EQ(parsed.particleCount, 128u);
    EXPECT_EQ(parsed.solver, "octree_gpu");
    EXPECT_EQ(parsed.integrator, "euler");
    EXPECT_FLOAT_EQ(parsed.totalEnergy, 5.0f);
    EXPECT_TRUE(parsed.energyEstimated);
}

TEST(BackendProtocolCodecTest, TST_UNT_PROT_003_RejectsMalformedSnapshotPayload)
{
    const std::string raw =
        "{\"ok\":true,\"cmd\":\"get_snapshot\",\"has_snapshot\":true,\"count\":1,"
        "\"particles\":[[1,2,3,4,5]]}";

    grav_protocol::BackendSnapshotPayload parsed{};
    std::string error;
    EXPECT_FALSE(grav_protocol::BackendJsonCodec::parseSnapshotResponse(raw, parsed, error));
    EXPECT_EQ(error, "invalid snapshot payload");
}

TEST(BackendProtocolCodecTest, TST_UNT_PROT_004_ParsesErrorEnvelopeForControlCommand)
{
    const std::string raw = grav_protocol::BackendJsonCodec::makeErrorResponse(
        grav_protocol::SetIntegrator,
        "invalid integrator value");

    grav_protocol::BackendResponseEnvelope parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::BackendJsonCodec::parseResponseEnvelope(raw, parsed, error)) << error;
    EXPECT_FALSE(parsed.ok);
    EXPECT_EQ(parsed.cmd, grav_protocol::SetIntegrator);
    EXPECT_EQ(parsed.error, "invalid integrator value");
}

} // namespace grav_test_protocol_codec
