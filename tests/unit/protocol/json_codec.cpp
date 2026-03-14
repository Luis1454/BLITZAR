#include "protocol/ServerJsonCodec.hpp"
#include "protocol/ServerProtocol.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace grav_test_protocol_codec {

TEST(ServerProtocolCodecTest, TST_UNT_PROT_001_ParsesCommandEnvelopeWithEscapedToken)
{
    grav_protocol::ServerCommandRequest request{};
    request.cmd = std::string(grav_protocol::SetSolver);
    request.token = "secret\"token";

    const std::string json = grav_protocol::ServerJsonCodec::makeCommandRequest(
        request,
        "\"value\":\"octree_gpu\"");

    grav_protocol::ServerCommandRequest parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseCommandRequest(json, parsed, error)) << error;
    EXPECT_EQ(parsed.cmd, grav_protocol::SetSolver);
    EXPECT_EQ(parsed.token, "secret\"token");
}

TEST(ServerProtocolCodecTest, TST_UNT_PROT_002_ParsesTypedStatusPayload)
{
    SimulationStats stats{};
    stats.steps = 42u;
    stats.dt = 0.02f;
    stats.paused = true;
    stats.faulted = true;
    stats.faultStep = 41u;
    stats.faultReason = "bad\\nstate";
    stats.sphEnabled = true;
    stats.serverFps = 144.5f;
    stats.performanceProfile = "interactive";
    stats.substepTargetDt = 0.0025f;
    stats.substepDt = 0.00125f;
    stats.substeps = 8u;
    stats.maxSubsteps = 32u;
    stats.snapshotPublishPeriodMs = 50u;
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

    const std::string raw = grav_protocol::ServerJsonCodec::makeStatusResponse(stats);

    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error)) << error;
    ASSERT_TRUE(parsed.envelope.ok);
    EXPECT_EQ(parsed.steps, 42u);
    EXPECT_FLOAT_EQ(parsed.dt, 0.02f);
    EXPECT_TRUE(parsed.paused);
    EXPECT_TRUE(parsed.faulted);
    EXPECT_EQ(parsed.faultStep, 41u);
    EXPECT_EQ(parsed.faultReason, "bad\\nstate");
    EXPECT_TRUE(parsed.sphEnabled);
    EXPECT_FLOAT_EQ(parsed.serverFps, 144.5f);
    EXPECT_EQ(parsed.performanceProfile, "interactive");
    EXPECT_FLOAT_EQ(parsed.substepTargetDt, 0.0025f);
    EXPECT_FLOAT_EQ(parsed.substepDt, 0.00125f);
    EXPECT_EQ(parsed.substeps, 8u);
    EXPECT_EQ(parsed.maxSubsteps, 32u);
    EXPECT_EQ(parsed.snapshotPublishPeriodMs, 50u);
    EXPECT_EQ(parsed.particleCount, 128u);
    EXPECT_EQ(parsed.solver, "octree_gpu");
    EXPECT_EQ(parsed.integrator, "euler");
    EXPECT_FLOAT_EQ(parsed.totalEnergy, 5.0f);
    EXPECT_TRUE(parsed.energyEstimated);
}

TEST(ServerProtocolCodecTest, TST_UNT_PROT_003_RejectsMalformedSnapshotPayload)
{
    const std::string raw =
        "{\"ok\":true,\"cmd\":\"get_snapshot\",\"has_snapshot\":true,\"count\":1,"
        "\"particles\":[[1,2,3,4,5]]}";

    grav_protocol::ServerSnapshotPayload parsed{};
    std::string error;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, parsed, error));
    EXPECT_EQ(error, "invalid snapshot payload");
}

TEST(ServerProtocolCodecTest, TST_UNT_PROT_004_ParsesErrorEnvelopeForControlCommand)
{
    const std::string raw = grav_protocol::ServerJsonCodec::makeErrorResponse(
        grav_protocol::SetIntegrator,
        "invalid integrator value");

    grav_protocol::ServerResponseEnvelope parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseResponseEnvelope(raw, parsed, error)) << error;
    EXPECT_FALSE(parsed.ok);
    EXPECT_EQ(parsed.cmd, grav_protocol::SetIntegrator);
    EXPECT_EQ(parsed.error, "invalid integrator value");
}

TEST(ServerProtocolCodecTest, TST_UNT_PROT_005_ExportsCurrentSchemaVersionLabel)
{
    EXPECT_EQ(grav_protocol::SchemaVersion, "server-json-v1");
    EXPECT_EQ(grav_protocol::Status, "status");
    EXPECT_EQ(grav_protocol::GetSnapshot, "get_snapshot");
    EXPECT_EQ(grav_protocol::SetSnapshotPublishCadence, "set_snapshot_publish_cadence");
}

} // namespace grav_test_protocol_codec
