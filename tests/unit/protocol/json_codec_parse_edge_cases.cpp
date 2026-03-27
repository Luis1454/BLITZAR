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

TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_047_ParseCommandRequestNormalizesWhitespaceAndCase)
{
    std::string error;
    grav_protocol::ServerCommandRequest request;

    EXPECT_TRUE(grav_protocol::ServerJsonCodec::parseCommandRequest(R"({"cmd": "  ReSeT  ", "token": "A-1"})", request, error));
    EXPECT_EQ(request.cmd, "reset");
    EXPECT_EQ(request.token, "A-1");
    EXPECT_TRUE(error.empty());
}

TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_048_ParseCommandRequestRejectsWhitespaceOnlyCommand)
{
    std::string error;
    grav_protocol::ServerCommandRequest request;

    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseCommandRequest(R"({"cmd": "   "})", request, error));
    EXPECT_EQ(error, "missing cmd");
}

TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_049_ParseResponseEnvelopeRejectsInvalidJsonAndBoolToken)
{
    std::string error;
    grav_protocol::ServerResponseEnvelope envelope;

    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseResponseEnvelope("[]", envelope, error));
    EXPECT_EQ(error, "invalid json");

    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseResponseEnvelope(R"({"ok": maybe, "cmd": "status"})", envelope, error));
    EXPECT_EQ(error, "invalid response");
}

TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_050_ReadStringAndBoolRejectMalformedTokens)
{
    std::string stringValue;
    bool boolValue = false;

    EXPECT_FALSE(grav_protocol::ServerJsonCodec::readString(R"({"key":"unterminated})", "key", stringValue));

    EXPECT_FALSE(grav_protocol::ServerJsonCodec::readBool(R"({"flag":"true"})", "flag", boolValue));
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readBool(R"({"flag": TRUE})", "flag", boolValue));
    EXPECT_TRUE(boolValue);
}

TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_051_ParseCommandRequestRejectsMissingColonSyntax)
{
    std::string error;
    grav_protocol::ServerCommandRequest request;

    EXPECT_FALSE(grav_protocol::ServerJsonCodec::parseCommandRequest(R"({"cmd" "status"})", request, error));
    EXPECT_EQ(error, "missing cmd");
}

TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_052_ReadStringSupportsEscapedQuotes)
{
    std::string value;
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readString("{\"text\": \"a \\\"quoted\\\" value\"}", "text", value));
    EXPECT_EQ(value, "a \"quoted\" value");
}

TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_053_ReadBoolRejectsEmptyToken)
{
    bool value = true;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::readBool(R"({"ok": })", "ok", value));
    EXPECT_TRUE(value);
}

TEST(ServerProtocolCodecParseEdgeTest, TST_UNT_PROT_054_ParseResponseEnvelopeKeepsProvidedErrorMessage)
{
    std::string error;
    grav_protocol::ServerResponseEnvelope envelope;

    EXPECT_TRUE(grav_protocol::ServerJsonCodec::parseResponseEnvelope(R"({"ok": false, "error": "boom"})", envelope, error));
    EXPECT_FALSE(envelope.ok);
    EXPECT_EQ(envelope.error, "boom");
    EXPECT_TRUE(error.empty());
}
