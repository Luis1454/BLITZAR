#include "command/CommandParser.hpp"
#include <gtest/gtest.h>
#include <string>
namespace grav_test_module_cli_command_parser {
TEST(CommandParserTest, TST_UNT_MODCLI_006_ParsesQuotedPathsAndComments)
{
    const std::string script = "  # heading\n"
                               "\n"
                               "load_config \"configs/test scene.ini\" # keep\n"
                               "export_snapshot \"exports/final state.vtk\" vtk\n"
                               "save_checkpoint \"checkpoints/final state.chk\"\n"
                               "load_checkpoint \"checkpoints/final state.chk\"\n";
    const grav_cmd::CommandParseResult result = grav_cmd::CommandParser::parseScript(script);
    ASSERT_TRUE(result.ok);
    ASSERT_EQ(result.requests.size(), 4u);
    EXPECT_EQ(result.requests[0].name, "load_config");
    EXPECT_EQ(std::get<std::string>(result.requests[0].arguments[0]), "configs/test scene.ini");
    EXPECT_EQ(result.requests[1].name, "export_snapshot");
    EXPECT_EQ(std::get<std::string>(result.requests[1].arguments[0]), "exports/final state.vtk");
    EXPECT_EQ(std::get<std::string>(result.requests[1].arguments[1]), "vtk");
    EXPECT_EQ(result.requests[2].name, "save_checkpoint");
    EXPECT_EQ(std::get<std::string>(result.requests[2].arguments[0]),
              "checkpoints/final state.chk");
    EXPECT_EQ(result.requests[3].name, "load_checkpoint");
    EXPECT_EQ(std::get<std::string>(result.requests[3].arguments[0]),
              "checkpoints/final state.chk");
}
TEST(CommandParserTest, TST_UNT_MODCLI_007_RejectsUnknownCommandWithLineNumber)
{
    const grav_cmd::CommandParseResult result =
        grav_cmd::CommandParser::parseScript("help\nbogus\n");
    ASSERT_FALSE(result.ok);
    EXPECT_EQ(result.error, "line 2: unknown command 'bogus'");
}
TEST(CommandParserTest, TST_UNT_MODCLI_008_RejectsWrongArityAndInvalidNumericValues)
{
    const grav_cmd::CommandParseResult arity =
        grav_cmd::CommandParser::parseLine("connect 127.0.0.1", 4u);
    ASSERT_FALSE(arity.ok);
    EXPECT_EQ(arity.error, "line 4: wrong arity for 'connect'");
    const grav_cmd::CommandParseResult integer =
        grav_cmd::CommandParser::parseLine("step nope", 6u);
    ASSERT_FALSE(integer.ok);
    EXPECT_EQ(integer.error, "line 6: invalid integer 'nope'");
    const grav_cmd::CommandParseResult floating =
        grav_cmd::CommandParser::parseLine("run_until soon", 9u);
    ASSERT_FALSE(floating.ok);
    EXPECT_EQ(floating.error, "line 9: invalid float 'soon'");
}
TEST(CommandParserTest, TST_UNT_MODCLI_028_ParseScriptKeepsTailLineWithoutTrailingNewline)
{
    const grav_cmd::CommandParseResult parsed =
        grav_cmd::CommandParser::parseScript("status\nstep 2");
    ASSERT_TRUE(parsed.ok);
    ASSERT_EQ(parsed.requests.size(), 2u);
    EXPECT_EQ(parsed.requests[0].name, "status");
    EXPECT_EQ(parsed.requests[1].name, "step");
    ASSERT_EQ(parsed.requests[1].arguments.size(), 1u);
    EXPECT_EQ(std::get<std::uint64_t>(parsed.requests[1].arguments[0]), 2u);
}
TEST(CommandParserTest, TST_UNT_MODCLI_029_ParseLineKeepsHashInsideQuotedToken)
{
    const grav_cmd::CommandParseResult parsed = grav_cmd::CommandParser::parseLine(
        "load_config \"cfg#release.ini\" # trailing comment", 12u);
    ASSERT_TRUE(parsed.ok);
    ASSERT_EQ(parsed.requests.size(), 1u);
    ASSERT_EQ(parsed.requests[0].arguments.size(), 1u);
    EXPECT_EQ(parsed.requests[0].name, "load_config");
    EXPECT_EQ(std::get<std::string>(parsed.requests[0].arguments[0]), "cfg#release.ini");
}
TEST(CommandParserTest, TST_UNT_MODCLI_030_ParseScriptStopsAtFirstInvalidLine)
{
    const std::string script = "status\n"
                               "run_until 3.0\n"
                               "run_steps nope\n"
                               "help\n";
    const grav_cmd::CommandParseResult parsed = grav_cmd::CommandParser::parseScript(script);
    ASSERT_FALSE(parsed.ok);
    EXPECT_EQ(parsed.error, "line 3: invalid integer 'nope'");
}
TEST(CommandParserTest, TST_UNT_MODCLI_033_ParseLineAcceptsCommentOnlyAndWhitespace)
{
    const grav_cmd::CommandParseResult parsed =
        grav_cmd::CommandParser::parseLine("   # only comment", 2u);
    ASSERT_TRUE(parsed.ok);
    EXPECT_TRUE(parsed.requests.empty());
}
TEST(CommandParserTest, TST_UNT_MODCLI_034_ParseLineSupportsSingleQuotedTokens)
{
    const grav_cmd::CommandParseResult parsed =
        grav_cmd::CommandParser::parseLine("load_config 'configs/release build.ini'", 7u);
    ASSERT_TRUE(parsed.ok);
    ASSERT_EQ(parsed.requests.size(), 1u);
    ASSERT_EQ(parsed.requests[0].arguments.size(), 1u);
    EXPECT_EQ(std::get<std::string>(parsed.requests[0].arguments[0]), "configs/release build.ini");
}
TEST(CommandParserTest, TST_UNT_MODCLI_035_ParseLineRejectsIntegerWithTrailingGarbage)
{
    const grav_cmd::CommandParseResult parsed =
        grav_cmd::CommandParser::parseLine("step 12ms", 21u);
    ASSERT_FALSE(parsed.ok);
    EXPECT_EQ(parsed.error, "line 21: invalid integer '12ms'");
}
TEST(CommandParserTest, TST_UNT_MODCLI_036_ParseLineRejectsFloatWithTrailingGarbage)
{
    const grav_cmd::CommandParseResult parsed =
        grav_cmd::CommandParser::parseLine("run_until 1.5s", 22u);
    ASSERT_FALSE(parsed.ok);
    EXPECT_EQ(parsed.error, "line 22: invalid float '1.5s'");
}
} // namespace grav_test_module_cli_command_parser
