// File: tests/unit/protocol/json_codec_parse.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "protocol/ServerJsonCodec.hpp"
#include <gtest/gtest.h>
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_031_ParseCommandRequestValid)
{
    std::string err;
    grav_protocol::ServerCommandRequest req;
    bool ok = grav_protocol::ServerJsonCodec::parseCommandRequest(
        R"({"cmd": "STATUS", "token": "xyz"})", req, err);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(req.cmd, "status");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(req.token, "xyz");
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_032_ParseCommandRequestMissingCmd)
{
    std::string err;
    grav_protocol::ServerCommandRequest req;
    bool ok = grav_protocol::ServerJsonCodec::parseCommandRequest(R"({"token": "xyz"})", req, err);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(err, "missing cmd");
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_033_ParseResponseEnvelopeValidOk)
{
    std::string err;
    grav_protocol::ServerResponseEnvelope env;
    bool ok = grav_protocol::ServerJsonCodec::parseResponseEnvelope(
        R"({"ok": true, "cmd": "TEST"})", env, err);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(ok);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(env.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(env.cmd, "TEST");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(env.error, "");
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_034_ParseResponseEnvelopeError)
{
    std::string err;
    grav_protocol::ServerResponseEnvelope env;
    bool ok = grav_protocol::ServerJsonCodec::parseResponseEnvelope(
        R"({"ok": false, "cmd": "TEST", "error": "broken"})", env, err);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(ok);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(env.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(env.cmd, "TEST");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(env.error, "broken");
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_035_ParseResponseEnvelopeErrorDefault)
{
    std::string err;
    grav_protocol::ServerResponseEnvelope env;
    bool ok = grav_protocol::ServerJsonCodec::parseResponseEnvelope(
        R"({"ok": false, "cmd": "TEST"})", env, err);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(ok);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(env.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(env.cmd, "TEST");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(env.error, "server error");
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_036_ParseResponseEnvelopeInvalid)
{
    std::string err;
    grav_protocol::ServerResponseEnvelope env;
    bool ok = grav_protocol::ServerJsonCodec::parseResponseEnvelope(R"({"cmd": "TEST"})", env, err);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(err, "invalid response");
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_037_ReadStringExtractsValue)
{
    std::string out;
    bool ok =
        grav_protocol::ServerJsonCodec::readString(R"({"key": "val", "other": "x"})", "key", out);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(out, "val");
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_038_ReadStringHandlesEscapes)
{
    std::string out;
    bool ok = grav_protocol::ServerJsonCodec::readString(
        R"({"key": "line\nbreak\rtest", "other": "x"})", "key", out);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(out, "line\nbreak\rtest");
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_039_ReadBoolExtractsValue)
{
    bool out = false;
    bool ok =
        grav_protocol::ServerJsonCodec::readBool(R"({"key": true, "other": false})", "key", out);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(ok);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(out);
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_040_ReadBoolHandlesFalse)
{
    bool out = true;
    bool ok =
        grav_protocol::ServerJsonCodec::readBool(R"({"key": false, "other": true})", "key", out);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(ok);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(out);
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_041_ReadNumberParsesSupportedNumericTypes)
{
    const std::string raw = R"({"i": -7, "u32": 42, "u64": 1234567890123, "f": 1.5, "d": -2.5e-3})";
    int i = 0;
    std::uint32_t u32 = 0u;
    std::uint64_t u64 = 0u;
    float f = 0.0f;
    double d = 0.0;
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readNumber(raw, "i", i));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readNumber(raw, "u32", u32));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readNumber(raw, "u64", u64));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readNumber(raw, "f", f));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readNumber(raw, "d", d));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(i, -7);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(u32, 42u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(u64, 1234567890123ull);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(f, 1.5f);
    /// Description: Executes the EXPECT_DOUBLE_EQ operation.
    EXPECT_DOUBLE_EQ(d, -2.5e-3);
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_042_ReadNumberReturnsFalseWhenKeyMissing)
{
    int out = 17;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::readNumber(R"({"other": 1})", "missing", out));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(out, 17);
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_043_ReadNumberRejectsInvalidIntegerToken)
{
    std::uint32_t out = 5u;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::readNumber(R"({"u32": -1})", "u32", out));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(out, 5u);
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_044_ReadNumberRejectsInvalidFloatToken)
{
    float out = 2.0f;
    EXPECT_FALSE(grav_protocol::ServerJsonCodec::readNumber(R"({"f": 1.2.3})", "f", out));
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(out, 2.0f);
}
/// Description: Executes the TEST operation.
TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_045_ReadNumberParsesScientificNotation)
{
    double out = 0.0;
    EXPECT_TRUE(grav_protocol::ServerJsonCodec::readNumber(R"({"d": 6.022e2})", "d", out));
    /// Description: Executes the EXPECT_DOUBLE_EQ operation.
    EXPECT_DOUBLE_EQ(out, 602.2);
}
