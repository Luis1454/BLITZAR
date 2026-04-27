// File: tests/unit/protocol/json_codec_parse_edge_cases.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "protocol/ServerJsonCodec.hpp"
#include <gtest/gtest.h>
#include <string>
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_046_ParseCommandRequestRejectsInvalidEnvelope)
{
    std::string error;
    grav_protocol::ServerCommandRequest request;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseCommandRequest("[]", request, error));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(error, "invalid json");
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_047_ParseCommandRequestNormalizesWhitespaceAndCase)
{
    std::string error;
    grav_protocol::ServerCommandRequest request;
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::parseCommandRequest(
        R"({"cmd": "  ReSeT  ", "token": "A-1"})", request, error));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(request.cmd, "reset");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(request.token, "A-1");
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(error.empty());
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_048_ParseCommandRequestRejectsWhitespaceOnlyCommand)
{
    std::string error;
    grav_protocol::ServerCommandRequest request;
    EXPECT_FALSE(
        grav_protocol::ServerJsonCodec::parseCommandRequest(R"({"cmd": "   "})", request, error));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(error, "missing cmd");
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_049_ParseResponseEnvelopeRejectsInvalidJsonAndBoolToken)
{
    std::string error;
    grav_protocol::ServerResponseEnvelope envelope;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseResponseEnvelope("[]", envelope, error));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(error, "invalid json");
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseResponseEnvelope(
        R"({"ok": maybe, "cmd": "status"})", envelope, error));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(error, "invalid response");
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_050_ReadStringAndBoolRejectMalformedTokens)
{
    std::string stringValue;
    bool boolValue = false;
    EXPECT_FALSE(
        grav_protocol::ServerJsonCodec::readString(R"({"key":"unterminated})", "key", stringValue));
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::readBool(R"({"flag":"true"})", "flag", boolValue));
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readBool(R"({"flag": TRUE})", "flag", boolValue));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(boolValue);
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_051_ParseCommandRequestRejectsMissingColonSyntax)
{
    std::string error;
    grav_protocol::ServerCommandRequest request;
    EXPECT_FALSE(
        grav_protocol::ServerJsonCodec::parseCommandRequest(R"({"cmd" "status"})", request, error));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(error, "missing cmd");
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_052_ReadStringSupportsEscapedQuotes)
{
    std::string value;
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readString("{\"text\": \"a \\\"quoted\\\" value\"}",
                                                           "text", value));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(value, "a \"quoted\" value");
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_053_ReadBoolRejectsEmptyToken)
{
    bool value = true;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::readBool(R"({"ok": })", "ok", value));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(value);
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_054_ParseResponseEnvelopeKeepsProvidedErrorMessage)
{
    std::string error;
    grav_protocol::ServerResponseEnvelope envelope;
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::parseResponseEnvelope(
        R"({"ok": false, "error": "boom"})", envelope, error));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(envelope.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(envelope.error, "boom");
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(error.empty());
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_055_ParseSnapshotResponseRejectsMissingParticlesArray)
{
    const std::string raw = R"({"ok":true,"cmd":"get_snapshot","has_snapshot":true})";
    grav_protocol::ServerSnapshotPayload payload{};
    std::string error;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, payload, error));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(error, "invalid snapshot payload");
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_056_ParseSnapshotResponseRejectsNonNumericParticleCoordinate)
{
    const std::string raw =
        R"({"ok":true,"cmd":"get_snapshot","has_snapshot":true,"particles":[[1,2,3,4,5,"hot"]]})";
    grav_protocol::ServerSnapshotPayload payload{};
    std::string error;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, payload, error));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(error, "invalid snapshot payload");
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_057_ParseStatusResponseAcceptsErrorEnvelopeWithoutMetrics)
{
    const std::string raw = R"({"ok":false,"cmd":"status","error":"faulted"})";
    grav_protocol::ServerStatusPayload payload{};
    std::string error;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, payload, error));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(payload.envelope.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(payload.envelope.error, "faulted");
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(error.empty());
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_058_ParseSnapshotResponseFallsBackSourceCountWhenMissing)
{
    const std::string raw =
        R"({"ok":true,"cmd":"get_snapshot","has_snapshot":true,"particles":[[1,2,3,4,5,6],[7,8,9,10,11,12]]})";
    grav_protocol::ServerSnapshotPayload payload{};
    std::string error;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, payload, error));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(payload.envelope.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(payload.particles.size(), 2u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(payload.sourceSize, 2u);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(error.empty());
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_059_ParseSnapshotResponseAcceptsEmptyParticlesArray)
{
    const std::string raw =
        R"({"ok":true,"cmd":"get_snapshot","has_snapshot":true,"particles":[]})";
    grav_protocol::ServerSnapshotPayload payload{};
    std::string error;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, payload, error));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(payload.envelope.ok);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(payload.hasSnapshot);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(payload.particles.empty());
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(payload.sourceSize, 0u);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(error.empty());
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_060_ParseSnapshotResponseAcceptsErrorEnvelopeWithoutSnapshotFields)
{
    const std::string raw = R"({"ok":false,"cmd":"get_snapshot","error":"snapshot unavailable"})";
    grav_protocol::ServerSnapshotPayload payload{};
    std::string error;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, payload, error));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(payload.envelope.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(payload.envelope.error, "snapshot unavailable");
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(payload.hasSnapshot);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(payload.particles.empty());
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(error.empty());
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_061_ParseStatusResponseIgnoresMalformedOptionalMetrics)
{
    const std::string raw =
        R"({"ok":true,"cmd":"status","steps":"oops","paused":true,"faulted":false,"total_time":"later","drift_pct":"nanish"})";
    grav_protocol::ServerStatusPayload payload{};
    std::string error;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, payload, error));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(payload.envelope.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(payload.steps, 0u);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(payload.paused);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(payload.faulted);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(payload.totalTime, 0.0f);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(payload.energyDriftPct, 0.0f);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(error.empty());
}
