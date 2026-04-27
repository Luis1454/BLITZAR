// File: tests/unit/protocol/json_codec.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

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
    const std::string json =
        grav_protocol::ServerJsonCodec::makeCommandRequest(request, "\"value\":\"octree_gpu\"");
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
    stats.totalTime = 1.25f;
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
    stats.gpuTelemetryEnabled = true;
    stats.gpuTelemetryAvailable = true;
    stats.gpuKernelMs = 1.5f;
    stats.gpuCopyMs = 0.75f;
    stats.gpuVramUsedBytes = 64u * 1024u * 1024u;
    stats.gpuVramTotalBytes = 256u * 1024u * 1024u;
    stats.exportQueueDepth = 3u;
    stats.exportActive = true;
    stats.exportCompletedCount = 9u;
    stats.exportFailedCount = 2u;
    stats.exportLastState = "writing";
    stats.exportLastPath = "exports/demo.vtk";
    stats.exportLastMessage = "background export active";
    const std::string raw = grav_protocol::ServerJsonCodec::makeStatusResponse(stats);
    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error)) << error;
    ASSERT_TRUE(parsed.envelope.ok);
    EXPECT_EQ(parsed.steps, 42u);
    EXPECT_FLOAT_EQ(parsed.dt, 0.02f);
    EXPECT_FLOAT_EQ(parsed.totalTime, 1.25f);
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
    EXPECT_TRUE(parsed.gpuTelemetryEnabled);
    EXPECT_TRUE(parsed.gpuTelemetryAvailable);
    EXPECT_FLOAT_EQ(parsed.gpuKernelMs, 1.5f);
    EXPECT_FLOAT_EQ(parsed.gpuCopyMs, 0.75f);
    EXPECT_EQ(parsed.gpuVramUsedBytes, 64u * 1024u * 1024u);
    EXPECT_EQ(parsed.gpuVramTotalBytes, 256u * 1024u * 1024u);
    EXPECT_EQ(parsed.exportQueueDepth, 3u);
    EXPECT_TRUE(parsed.exportActive);
    EXPECT_EQ(parsed.exportCompletedCount, 9u);
    EXPECT_EQ(parsed.exportFailedCount, 2u);
    EXPECT_EQ(parsed.exportLastState, "writing");
    EXPECT_EQ(parsed.exportLastPath, "exports/demo.vtk");
    EXPECT_EQ(parsed.exportLastMessage, "background export active");
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
TEST(ServerProtocolCodecTest, TST_UNT_PROT_006_ParsesSnapshotSourceCountMetadata)
{
    std::vector<RenderParticle> snapshot(2u);
    snapshot[0].x = 1.0f;
    snapshot[1].x = 2.0f;
    const std::string raw =
        grav_protocol::ServerJsonCodec::makeSnapshotResponse(true, snapshot, 8u);
    grav_protocol::ServerSnapshotPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, parsed, error)) << error;
    ASSERT_TRUE(parsed.envelope.ok);
    EXPECT_TRUE(parsed.hasSnapshot);
    EXPECT_EQ(parsed.particles.size(), 2u);
    EXPECT_EQ(parsed.sourceSize, 8u);
}
TEST(ServerProtocolCodecTest, TST_UNT_PROT_004_ParsesErrorEnvelopeForControlCommand)
{
    const std::string raw = grav_protocol::ServerJsonCodec::makeErrorResponse(
        grav_protocol::SetIntegrator, "invalid integrator value");
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
    EXPECT_EQ(grav_protocol::SaveCheckpoint, "save_checkpoint");
    EXPECT_EQ(grav_protocol::LoadCheckpoint, "load_checkpoint");
    EXPECT_EQ(grav_protocol::SetGpuTelemetry, "set_gpu_telemetry");
    EXPECT_EQ(grav_protocol::SetSnapshotPublishCadence, "set_snapshot_publish_cadence");
}
// TST_UNT_PROT_007: Number parsing with maximum float32 precision
TEST(ServerProtocolCodecTest, TST_UNT_PROT_007_ParsesFloatPrecisionBoundaries)
{
    SimulationStats stats{};
    stats.dt = 1.23456789f;     // High precision float
    stats.totalTime = 0.00001f; // Very small value
    stats.serverFps = 144.5f;
    stats.steps = 1u;
    const std::string raw = grav_protocol::ServerJsonCodec::makeStatusResponse(stats);
    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error)) << error;
    EXPECT_TRUE(parsed.envelope.ok);
    EXPECT_FLOAT_EQ(parsed.dt, 1.23456789f);
    EXPECT_FLOAT_EQ(parsed.totalTime, 0.00001f);
}
// TST_UNT_PROT_008: Very large numeric values in status
TEST(ServerProtocolCodecTest, TST_UNT_PROT_008_ParsesLargeNumericStatus)
{
    SimulationStats stats{};
    stats.steps = 999999999u;
    stats.particleCount = 1024u * 1024u;
    stats.gpuVramTotalBytes = 16u * 1024u * 1024u * 1024u;
    stats.totalTime = 1e6f;
    const std::string raw = grav_protocol::ServerJsonCodec::makeStatusResponse(stats);
    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error)) << error;
    EXPECT_EQ(parsed.steps, 999999999u);
    EXPECT_EQ(parsed.particleCount, 1024u * 1024u);
}
// TST_UNT_PROT_009: Zero and boundary values in numeric fields
TEST(ServerProtocolCodecTest, TST_UNT_PROT_009_ParsesBoundaryNumericValues)
{
    SimulationStats stats{};
    stats.dt = 0.0f;
    stats.serverFps = 0.0f;
    stats.kineticEnergy = -0.0f;
    stats.substeps = 1u;
    const std::string raw = grav_protocol::ServerJsonCodec::makeStatusResponse(stats);
    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error)) << error;
    EXPECT_TRUE(parsed.envelope.ok);
}
// TST_UNT_PROT_010: Status with extreme energy values
TEST(ServerProtocolCodecTest, TST_UNT_PROT_010_ParsesExtremeEnergyStatus)
{
    SimulationStats stats{};
    stats.kineticEnergy = 1e10f;
    stats.potentialEnergy = -1e10f;
    stats.thermalEnergy = 1e15f;
    stats.totalEnergy = 1e15f;
    const std::string raw = grav_protocol::ServerJsonCodec::makeStatusResponse(stats);
    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error)) << error;
    EXPECT_TRUE(parsed.envelope.ok);
}
// TST_UNT_PROT_011: Rejects malformed JSON with missing required fields
TEST(ServerProtocolCodecTest, TST_UNT_PROT_011_RejectsMalformedJsonMissingOkField)
{
    const std::string raw = "{\"cmd\":\"set_solver\"}"; // Missing "ok" field
    grav_protocol::ServerResponseEnvelope parsed{};
    std::string error;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseResponseEnvelope(raw, parsed, error));
}
// TST_UNT_PROT_012: Rejects incomplete JSON
TEST(ServerProtocolCodecTest, TST_UNT_PROT_012_RejectsIncompleteJson)
{
    const std::string raw = "{\"ok\":true,\"cmd\":\"get_status\""; // Incomplete
    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error));
}
// TST_UNT_PROT_013: Rejects JSON with wrong field types
TEST(ServerProtocolCodecTest, TST_UNT_PROT_013_RejectsWrongFieldTypes)
{
    const std::string raw = "{\"ok\":\"true\",\"cmd\":\"get_status\",\"steps\":\"not_a_number\"}";
    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error));
}
// TST_UNT_PROT_014: Handles empty JSON object
TEST(ServerProtocolCodecTest, TST_UNT_PROT_014_RejectsEmptyJsonObject)
{
    const std::string raw = "{}";
    grav_protocol::ServerResponseEnvelope parsed{};
    std::string error;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseResponseEnvelope(raw, parsed, error));
}
// TST_UNT_PROT_015: Rejects invalid JSON syntax
TEST(ServerProtocolCodecTest, TST_UNT_PROT_015_RejectsInvalidJsonSyntax)
{
    const std::string raw = "{ok: true, cmd: 'get_status'}"; // Invalid: unquoted keys/values
    grav_protocol::ServerResponseEnvelope parsed{};
    std::string error;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseResponseEnvelope(raw, parsed, error));
}
// TST_UNT_PROT_016: Status with minimal field set
TEST(ServerProtocolCodecTest, TST_UNT_PROT_016_ParsesMinimalStatusPayload)
{
    SimulationStats minimal_stats{};
    minimal_stats.steps = 0u;
    const std::string raw = grav_protocol::ServerJsonCodec::makeStatusResponse(minimal_stats);
    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error)) << error;
    EXPECT_TRUE(parsed.envelope.ok);
}
// TST_UNT_PROT_017: Status with all boolean flags set to true
TEST(ServerProtocolCodecTest, TST_UNT_PROT_017_ParsesAllBooleanFlagsTrue)
{
    SimulationStats stats{};
    stats.paused = true;
    stats.faulted = true;
    stats.sphEnabled = true;
    stats.energyEstimated = true;
    stats.gpuTelemetryEnabled = true;
    stats.gpuTelemetryAvailable = true;
    stats.exportActive = true;
    const std::string raw = grav_protocol::ServerJsonCodec::makeStatusResponse(stats);
    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error)) << error;
    EXPECT_TRUE(parsed.paused);
    EXPECT_TRUE(parsed.faulted);
    EXPECT_TRUE(parsed.sphEnabled);
    EXPECT_TRUE(parsed.energyEstimated);
}
// TST_UNT_PROT_018: Status with all boolean flags set to false
TEST(ServerProtocolCodecTest, TST_UNT_PROT_018_ParsesAllBooleanFlagsFalse)
{
    SimulationStats stats{};
    stats.paused = false;
    stats.faulted = false;
    stats.sphEnabled = false;
    stats.energyEstimated = false;
    stats.gpuTelemetryEnabled = false;
    stats.gpuTelemetryAvailable = false;
    stats.exportActive = false;
    const std::string raw = grav_protocol::ServerJsonCodec::makeStatusResponse(stats);
    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error)) << error;
    EXPECT_FALSE(parsed.paused);
    EXPECT_FALSE(parsed.faulted);
}
// TST_UNT_PROT_019: Status with special characters in string fields
TEST(ServerProtocolCodecTest, TST_UNT_PROT_019_ParsesStatusWithSpecialCharsInStrings)
{
    SimulationStats stats{};
    stats.faultReason = "error\nwith\\special\"chars\rhere";
    stats.exportLastState = "idle\t\ttab";
    stats.exportLastMessage = "message with \"quotes\" and 'apostrophes'";
    const std::string raw = grav_protocol::ServerJsonCodec::makeStatusResponse(stats);
    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error)) << error;
    EXPECT_TRUE(parsed.envelope.ok);
}
// TST_UNT_PROT_020: Empty strings in status fields
TEST(ServerProtocolCodecTest, TST_UNT_PROT_020_ParsesStatusWithEmptyStrings)
{
    SimulationStats stats{};
    stats.faultReason = "";
    stats.performanceProfile = "";
    stats.solverName = "";
    stats.integratorName = "";
    stats.exportLastMessage = "";
    const std::string raw = grav_protocol::ServerJsonCodec::makeStatusResponse(stats);
    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error)) << error;
    EXPECT_TRUE(parsed.envelope.ok);
}
// TST_UNT_PROT_021: Snapshot with single particle
TEST(ServerProtocolCodecTest, TST_UNT_PROT_021_ParsesSnapshotWithSingleParticle)
{
    std::vector<RenderParticle> snapshot(1u);
    snapshot[0].x = 1.0f;
    snapshot[0].y = 2.0f;
    snapshot[0].z = 3.0f;
    snapshot[0].mass = 4.0f;
    snapshot[0].pressureNorm = 0.1f;
    snapshot[0].temperature = 0.3f;
    const std::string raw =
        grav_protocol::ServerJsonCodec::makeSnapshotResponse(true, snapshot, 1u);
    grav_protocol::ServerSnapshotPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, parsed, error)) << error;
    EXPECT_EQ(parsed.particles.size(), 1u);
    EXPECT_EQ(parsed.sourceSize, 1u);
}
// TST_UNT_PROT_022: Snapshot with many particles
TEST(ServerProtocolCodecTest, TST_UNT_PROT_022_ParsesSnapshotWithManyParticles)
{
    std::vector<RenderParticle> snapshot(1000u);
    for (size_t i = 0u; i < snapshot.size(); ++i) {
        snapshot[i].x = static_cast<float>(i);
        snapshot[i].y = static_cast<float>(i + 1u);
        snapshot[i].z = static_cast<float>(i + 2u);
    }
    const std::string raw =
        grav_protocol::ServerJsonCodec::makeSnapshotResponse(true, snapshot, 1000u);
    grav_protocol::ServerSnapshotPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, parsed, error)) << error;
    EXPECT_EQ(parsed.particles.size(), 1000u);
    EXPECT_EQ(parsed.sourceSize, 1000u);
}
// TST_UNT_PROT_023: Snapshot with no snapshot data available
TEST(ServerProtocolCodecTest, TST_UNT_PROT_023_ParsesSnapshotUnavailable)
{
    std::vector<RenderParticle> empty_snapshot;
    const std::string raw =
        grav_protocol::ServerJsonCodec::makeSnapshotResponse(false, empty_snapshot, 0u);
    grav_protocol::ServerSnapshotPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, parsed, error)) << error;
    EXPECT_FALSE(parsed.hasSnapshot);
    EXPECT_EQ(parsed.particles.size(), 0u);
}
// TST_UNT_PROT_024: Snapshot with boundary coordinate values
TEST(ServerProtocolCodecTest, TST_UNT_PROT_024_ParsesSnapshotBoundaryCoordinates)
{
    std::vector<RenderParticle> snapshot(3u);
    snapshot[0].x = 0.0f;
    snapshot[0].y = 0.0f;
    snapshot[0].z = 0.0f;
    snapshot[1].x = 1e6f;
    snapshot[1].y = -1e6f;
    snapshot[1].z = 1e-6f;
    snapshot[2].x = -1e6f;
    snapshot[2].y = 1e6f;
    snapshot[2].z = -1e-6f;
    const std::string raw =
        grav_protocol::ServerJsonCodec::makeSnapshotResponse(true, snapshot, 3u);
    grav_protocol::ServerSnapshotPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, parsed, error)) << error;
    ASSERT_EQ(parsed.particles.size(), 3u);
    EXPECT_FLOAT_EQ(parsed.particles[0].x, 0.0f);
    EXPECT_FLOAT_EQ(parsed.particles[1].x, 1e6f);
}
// TST_UNT_PROT_025: Rejects snapshot with mismatched particle count
TEST(ServerProtocolCodecTest, TST_UNT_PROT_025_RejectsSnapshotArrayMismatch)
{
    // Manually craft JSON with mismatch between count and actual array size
    const std::string raw =
        "{\"ok\":true,\"cmd\":\"get_snapshot\",\"has_snapshot\":true,\"count\":10,"
        "\"particles\":[[1,2,3,4,5,6,7]]}"; // Only 1 particle, but count says 10
    grav_protocol::ServerSnapshotPayload parsed{};
    std::string error;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, parsed, error));
}
// TST_UNT_PROT_026: Command request round-trip with empty token
TEST(ServerProtocolCodecTest, TST_UNT_PROT_026_CommandRequestWithEmptyToken)
{
    grav_protocol::ServerCommandRequest request{};
    request.cmd = std::string(grav_protocol::SetSolver);
    request.token = "";
    const std::string json =
        grav_protocol::ServerJsonCodec::makeCommandRequest(request, "\"value\":\"octree_gpu\"");
    grav_protocol::ServerCommandRequest parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseCommandRequest(json, parsed, error)) << error;
    EXPECT_EQ(parsed.token, "");
}
// TST_UNT_PROT_027: Command request with very long token
TEST(ServerProtocolCodecTest, TST_UNT_PROT_027_CommandRequestWithLongToken)
{
    grav_protocol::ServerCommandRequest request{};
    request.cmd = std::string(grav_protocol::SetSolver);
    request.token = std::string(1000u, 'x');
    const std::string json =
        grav_protocol::ServerJsonCodec::makeCommandRequest(request, "\"value\":\"octree_gpu\"");
    grav_protocol::ServerCommandRequest parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseCommandRequest(json, parsed, error)) << error;
    EXPECT_EQ(parsed.token.size(), 1000u);
}
// TST_UNT_PROT_028: Error response with various command types
TEST(ServerProtocolCodecTest, TST_UNT_PROT_028_ErrorResponseWithDifferentCommands)
{
    const auto test_command = [](std::string_view cmd) {
        const std::string raw =
            grav_protocol::ServerJsonCodec::makeErrorResponse(cmd, "test error");
        grav_protocol::ServerResponseEnvelope parsed{};
        std::string error;
        EXPECT_TRUE(grav_protocol::ServerJsonCodec::parseResponseEnvelope(raw, parsed, error))
            << error;
        EXPECT_FALSE(parsed.ok);
        EXPECT_EQ(parsed.cmd, cmd);
        EXPECT_EQ(parsed.error, "test error");
    };
    test_command(grav_protocol::SetSolver);
    test_command(grav_protocol::SetIntegrator);
    test_command(grav_protocol::Status);
    test_command(grav_protocol::GetSnapshot);
}
// TST_UNT_PROT_029: String escaping preserves all special sequences
TEST(ServerProtocolCodecTest, TST_UNT_PROT_029_StringEscapingRoundTrip)
{
    SimulationStats stats{};
    stats.faultReason = "Complex: \\ \" \n \r \t special";
    const std::string raw = grav_protocol::ServerJsonCodec::makeStatusResponse(stats);
    grav_protocol::ServerStatusPayload parsed{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, parsed, error)) << error;
    EXPECT_EQ(parsed.faultReason, "Complex: \\ \" \n \r \t special");
}
// TST_UNT_PROT_030: Consecutive rapid command-response cycles
TEST(ServerProtocolCodecTest, TST_UNT_PROT_030_ConsecutiveCommandResponseCycles)
{
    for (int i = 0; i < 10; ++i) {
        grav_protocol::ServerCommandRequest request{};
        request.cmd = std::string(grav_protocol::SetSolver);
        request.token = "token_" + std::to_string(i);
        const std::string json =
            grav_protocol::ServerJsonCodec::makeCommandRequest(request, "\"value\":\"octree_gpu\"");
        grav_protocol::ServerCommandRequest parsed{};
        std::string error;
        ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseCommandRequest(json, parsed, error))
            << error;
        const std::string response =
            grav_protocol::ServerJsonCodec::makeOkResponse(grav_protocol::SetSolver);
        grav_protocol::ServerResponseEnvelope parsed_resp{};
        ASSERT_TRUE(
            grav_protocol::ServerJsonCodec::parseResponseEnvelope(response, parsed_resp, error))
            << error;
        EXPECT_TRUE(parsed_resp.ok);
    }
}
} // namespace grav_test_protocol_codec
