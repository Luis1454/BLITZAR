#include "protocol/ServerJsonCodec.hpp"
#include <gtest/gtest.h>

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_031_ParseCommandRequestValid) {
    std::string err;
    grav_protocol::ServerCommandRequest req;
    bool ok = grav_protocol::ServerJsonCodec::parseCommandRequest(R"({"cmd": "STATUS", "token": "xyz"})", req, err);
    EXPECT_TRUE(ok);
    EXPECT_EQ(req.cmd, "status");
    EXPECT_EQ(req.token, "xyz");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_032_ParseCommandRequestMissingCmd) {
    std::string err;
    grav_protocol::ServerCommandRequest req;
    bool ok = grav_protocol::ServerJsonCodec::parseCommandRequest(R"({"token": "xyz"})", req, err);
    EXPECT_FALSE(ok);
    EXPECT_EQ(err, "missing cmd");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_033_ParseResponseEnvelopeValidOk) {
    std::string err;
    grav_protocol::ServerResponseEnvelope env;
    bool ok = grav_protocol::ServerJsonCodec::parseResponseEnvelope(R"({"ok": true, "cmd": "TEST"})", env, err);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(env.ok);
    EXPECT_EQ(env.cmd, "TEST");
    EXPECT_EQ(env.error, "");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_034_ParseResponseEnvelopeError) {
    std::string err;
    grav_protocol::ServerResponseEnvelope env;
    bool ok = grav_protocol::ServerJsonCodec::parseResponseEnvelope(R"({"ok": false, "cmd": "TEST", "error": "broken"})", env, err);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(env.ok);
    EXPECT_EQ(env.cmd, "TEST");
    EXPECT_EQ(env.error, "broken");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_035_ParseResponseEnvelopeErrorDefault) {
    std::string err;
    grav_protocol::ServerResponseEnvelope env;
    bool ok = grav_protocol::ServerJsonCodec::parseResponseEnvelope(R"({"ok": false, "cmd": "TEST"})", env, err);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(env.ok);
    EXPECT_EQ(env.cmd, "TEST");
    EXPECT_EQ(env.error, "server error");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_036_ParseResponseEnvelopeInvalid) {
    std::string err;
    grav_protocol::ServerResponseEnvelope env;
    bool ok = grav_protocol::ServerJsonCodec::parseResponseEnvelope(R"({"cmd": "TEST"})", env, err);
    EXPECT_FALSE(ok);
    EXPECT_EQ(err, "invalid response");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_037_ReadStringExtractsValue) {
    std::string out;
    bool ok = grav_protocol::ServerJsonCodec::readString(R"({"key": "val", "other": "x"})", "key", out);
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, "val");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_038_ReadStringHandlesEscapes) {
    std::string out;
    bool ok = grav_protocol::ServerJsonCodec::readString(R"({"key": "line\nbreak\rtest", "other": "x"})", "key", out);
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, "line\nbreak\rtest");
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_039_ReadBoolExtractsValue) {
    bool out = false;
    bool ok = grav_protocol::ServerJsonCodec::readBool(R"({"key": true, "other": false})", "key", out);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(out);
}

TEST(ServerProtocolCodecParseTest, TST_UNT_PROT_040_ReadBoolHandlesFalse) {
    bool out = true;
    bool ok = grav_protocol::ServerJsonCodec::readBool(R"({"key": false, "other": true})", "key", out);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(out);
}

