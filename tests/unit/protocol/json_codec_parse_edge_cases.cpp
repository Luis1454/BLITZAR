// File: tests/unit/protocol/json_codec_parse_edge_cases.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "protocol/ServerJsonCodec.hpp"
#include <gtest/gtest.h>
#include <string>
TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_046_ParseCommandRequestRejectsInvalidEnvelope)
{
    std::string error;
    grav_protocol::ServerCommandRequest request;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseCommandRequest("[]", request, error));
    EXPECT_EQ(error, "invalid json");
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_047_ParseCommandRequestNormalizesWhitespaceAndCase)
{
    std::string error;
    grav_protocol::ServerCommandRequest request;
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::parseCommandRequest(
        R"({"cmd": "  ReSeT  ", "token": "A-1"})", request, error));
    EXPECT_EQ(request.cmd, "reset");
    EXPECT_EQ(request.token, "A-1");
    EXPECT_TRUE(error.empty());
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_048_ParseCommandRequestRejectsWhitespaceOnlyCommand)
{
    std::string error;
    grav_protocol::ServerCommandRequest request;
    EXPECT_FALSE(
        grav_protocol::ServerJsonCodec::parseCommandRequest(R"({"cmd": "   "})", request, error));
    EXPECT_EQ(error, "missing cmd");
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_049_ParseResponseEnvelopeRejectsInvalidJsonAndBoolToken)
{
    std::string error;
    grav_protocol::ServerResponseEnvelope envelope;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseResponseEnvelope("[]", envelope, error));
    EXPECT_EQ(error, "invalid json");
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseResponseEnvelope(
        R"({"ok": maybe, "cmd": "status"})", envelope, error));
    EXPECT_EQ(error, "invalid response");
}
TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_050_ReadStringAndBoolRejectMalformedTokens)
{
    std::string stringValue;
    bool boolValue = false;
    EXPECT_FALSE(
        grav_protocol::ServerJsonCodec::readString(R"({"key":"unterminated})", "key", stringValue));
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::readBool(R"({"flag":"true"})", "flag", boolValue));
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readBool(R"({"flag": TRUE})", "flag", boolValue));
    EXPECT_TRUE(boolValue);
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_051_ParseCommandRequestRejectsMissingColonSyntax)
{
    std::string error;
    grav_protocol::ServerCommandRequest request;
    EXPECT_FALSE(
        grav_protocol::ServerJsonCodec::parseCommandRequest(R"({"cmd" "status"})", request, error));
    EXPECT_EQ(error, "missing cmd");
}
TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_052_ReadStringSupportsEscapedQuotes)
{
    std::string value;
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readString("{\"text\": \"a \\\"quoted\\\" value\"}",
                                                           "text", value));
    EXPECT_EQ(value, "a \"quoted\" value");
}
TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_053_ReadBoolRejectsEmptyToken)
{
    bool value = true;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::readBool(R"({"ok": })", "ok", value));
    EXPECT_TRUE(value);
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_054_ParseResponseEnvelopeKeepsProvidedErrorMessage)
{
    std::string error;
    grav_protocol::ServerResponseEnvelope envelope;
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::parseResponseEnvelope(
        R"({"ok": false, "error": "boom"})", envelope, error));
    EXPECT_FALSE(envelope.ok);
    EXPECT_EQ(envelope.error, "boom");
    EXPECT_TRUE(error.empty());
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_055_ParseSnapshotResponseRejectsMissingParticlesArray)
{
    const std::string raw = R"({"ok":true,"cmd":"get_snapshot","has_snapshot":true})";
    grav_protocol::ServerSnapshotPayload payload{};
    std::string error;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, payload, error));
    EXPECT_EQ(error, "invalid snapshot payload");
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_056_ParseSnapshotResponseRejectsNonNumericParticleCoordinate)
{
    const std::string raw =
        R"({"ok":true,"cmd":"get_snapshot","has_snapshot":true,"particles":[[1,2,3,4,5,"hot"]]})";
    grav_protocol::ServerSnapshotPayload payload{};
    std::string error;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, payload, error));
    EXPECT_EQ(error, "invalid snapshot payload");
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_057_ParseStatusResponseAcceptsErrorEnvelopeWithoutMetrics)
{
    const std::string raw = R"({"ok":false,"cmd":"status","error":"faulted"})";
    grav_protocol::ServerStatusPayload payload{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, payload, error));
    EXPECT_FALSE(payload.envelope.ok);
    EXPECT_EQ(payload.envelope.error, "faulted");
    EXPECT_TRUE(error.empty());
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_058_ParseSnapshotResponseFallsBackSourceCountWhenMissing)
{
    const std::string raw =
        R"({"ok":true,"cmd":"get_snapshot","has_snapshot":true,"particles":[[1,2,3,4,5,6],[7,8,9,10,11,12]]})";
    grav_protocol::ServerSnapshotPayload payload{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, payload, error));
    EXPECT_TRUE(payload.envelope.ok);
    EXPECT_EQ(payload.particles.size(), 2u);
    EXPECT_EQ(payload.sourceSize, 2u);
    EXPECT_TRUE(error.empty());
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_059_ParseSnapshotResponseAcceptsEmptyParticlesArray)
{
    const std::string raw =
        R"({"ok":true,"cmd":"get_snapshot","has_snapshot":true,"particles":[]})";
    grav_protocol::ServerSnapshotPayload payload{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, payload, error));
    EXPECT_TRUE(payload.envelope.ok);
    EXPECT_TRUE(payload.hasSnapshot);
    EXPECT_TRUE(payload.particles.empty());
    EXPECT_EQ(payload.sourceSize, 0u);
    EXPECT_TRUE(error.empty());
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_060_ParseSnapshotResponseAcceptsErrorEnvelopeWithoutSnapshotFields)
{
    const std::string raw = R"({"ok":false,"cmd":"get_snapshot","error":"snapshot unavailable"})";
    grav_protocol::ServerSnapshotPayload payload{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseSnapshotResponse(raw, payload, error));
    EXPECT_FALSE(payload.envelope.ok);
    EXPECT_EQ(payload.envelope.error, "snapshot unavailable");
    EXPECT_FALSE(payload.hasSnapshot);
    EXPECT_TRUE(payload.particles.empty());
    EXPECT_TRUE(error.empty());
}
TEST(ServerProtocolCodecParseEdgeTest,
     TST_UNT_PROT_061_ParseStatusResponseIgnoresMalformedOptionalMetrics)
{
    const std::string raw =
        R"({"ok":true,"cmd":"status","steps":"oops","paused":true,"faulted":false,"total_time":"later","drift_pct":"nanish"})";
    grav_protocol::ServerStatusPayload payload{};
    std::string error;
    ASSERT_TRUE(grav_protocol::ServerJsonCodec::parseStatusResponse(raw, payload, error));
    EXPECT_TRUE(payload.envelope.ok);
    EXPECT_EQ(payload.steps, 0u);
    EXPECT_TRUE(payload.paused);
    EXPECT_FALSE(payload.faulted);
    EXPECT_FLOAT_EQ(payload.totalTime, 0.0f);
    EXPECT_FLOAT_EQ(payload.energyDriftPct, 0.0f);
    EXPECT_TRUE(error.empty());
}
