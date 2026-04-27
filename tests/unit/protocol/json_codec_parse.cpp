/*
 * @file tests/unit/protocol/json_codec_parse.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "protocol/ServerJsonCodec.hpp"
#include <gtest/gtest.h>

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_031_ParseCommandRequestValid)
{
    std::string err;
    grav_protocol::ServerCommandRequest req;
    bool ok = grav_protocol::ServerJsonCodec::parseCommandRequest(
        R"({"cmd": "STATUS", "token": "xyz"})", req, err);
    EXPECT_TRUE(ok);
    EXPECT_EQ(req.cmd, "status");
    EXPECT_EQ(req.token, "xyz");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_032_ParseCommandRequestMissingCmd)
{
    std::string err;
    grav_protocol::ServerCommandRequest req;
    bool ok = grav_protocol::ServerJsonCodec::parseCommandRequest(R"({"token": "xyz"})", req, err);
    EXPECT_FALSE(ok);
    EXPECT_EQ(err, "missing cmd");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_033_ParseResponseEnvelopeValidOk)
{
    std::string err;
    grav_protocol::ServerResponseEnvelope env;
    bool ok = grav_protocol::ServerJsonCodec::parseResponseEnvelope(
        R"({"ok": true, "cmd": "TEST"})", env, err);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(env.ok);
    EXPECT_EQ(env.cmd, "TEST");
    EXPECT_EQ(env.error, "");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_034_ParseResponseEnvelopeError)
{
    std::string err;
    grav_protocol::ServerResponseEnvelope env;
    bool ok = grav_protocol::ServerJsonCodec::parseResponseEnvelope(
        R"({"ok": false, "cmd": "TEST", "error": "broken"})", env, err);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(env.ok);
    EXPECT_EQ(env.cmd, "TEST");
    EXPECT_EQ(env.error, "broken");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_035_ParseResponseEnvelopeErrorDefault)
{
    std::string err;
    grav_protocol::ServerResponseEnvelope env;
    bool ok = grav_protocol::ServerJsonCodec::parseResponseEnvelope(
        R"({"ok": false, "cmd": "TEST"})", env, err);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(env.ok);
    EXPECT_EQ(env.cmd, "TEST");
    EXPECT_EQ(env.error, "server error");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_036_ParseResponseEnvelopeInvalid)
{
    std::string err;
    grav_protocol::ServerResponseEnvelope env;
    bool ok = grav_protocol::ServerJsonCodec::parseResponseEnvelope(R"({"cmd": "TEST"})", env, err);
    EXPECT_FALSE(ok);
    EXPECT_EQ(err, "invalid response");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_037_ReadStringExtractsValue)
{
    std::string out;
    bool ok =
        grav_protocol::ServerJsonCodec::readString(R"({"key": "val", "other": "x"})", "key", out);
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, "val");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_038_ReadStringHandlesEscapes)
{
    std::string out;
    bool ok = grav_protocol::ServerJsonCodec::readString(
        R"({"key": "line\nbreak\rtest", "other": "x"})", "key", out);
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, "line\nbreak\rtest");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_039_ReadBoolExtractsValue)
{
    bool out = false;
    bool ok =
        grav_protocol::ServerJsonCodec::readBool(R"({"key": true, "other": false})", "key", out);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(out);
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_040_ReadBoolHandlesFalse)
{
    bool out = true;
    bool ok =
        grav_protocol::ServerJsonCodec::readBool(R"({"key": false, "other": true})", "key", out);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(out);
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_041_ReadNumberParsesSupportedNumericTypes)
{
    const std::string raw = R"({"i": -7, "u32": 42, "u64": 1234567890123, "f": 1.5, "d": -2.5e-3})";
    int i = 0;
    std::uint32_t u32 = 0u;
    std::uint64_t u64 = 0u;
    float f = 0.0f;
    double d = 0.0;
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readNumber(raw, "i", i));
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readNumber(raw, "u32", u32));
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readNumber(raw, "u64", u64));
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readNumber(raw, "f", f));
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readNumber(raw, "d", d));
    EXPECT_EQ(i, -7);
    EXPECT_EQ(u32, 42u);
    EXPECT_EQ(u64, 1234567890123ull);
    EXPECT_FLOAT_EQ(f, 1.5f);
    EXPECT_DOUBLE_EQ(d, -2.5e-3);
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_042_ReadNumberReturnsFalseWhenKeyMissing)
{
    int out = 17;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::readNumber(R"({"other": 1})", "missing", out));
    EXPECT_EQ(out, 17);
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_043_ReadNumberRejectsInvalidIntegerToken)
{
    std::uint32_t out = 5u;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::readNumber(R"({"u32": -1})", "u32", out));
    EXPECT_EQ(out, 5u);
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_044_ReadNumberRejectsInvalidFloatToken)
{
    float out = 2.0f;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::readNumber(R"({"f": 1.2.3})", "f", out));
    EXPECT_FLOAT_EQ(out, 2.0f);
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_045_ReadNumberParsesScientificNotation)
{
    double out = 0.0;
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readNumber(R"({"d": 6.022e2})", "d", out));
    EXPECT_DOUBLE_EQ(out, 602.2);
}
