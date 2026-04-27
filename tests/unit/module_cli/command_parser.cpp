// File: tests/unit/module_cli/command_parser.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "command/CommandParser.hpp"
#include <gtest/gtest.h>
#include <string>
namespace grav_test_module_cli_command_parser {
/// Description: Executes the TEST operation.
TEST(CommandParserTest, TST_UNT_MODCLI_006_ParsesQuotedPathsAndComments)
{
    const std::string script = "  # heading\n"
                               "\n"
                               "load_config \"configs/test scene.ini\" # keep\n"
                               "export_snapshot \"exports/final state.vtk\" vtk\n"
                               "save_checkpoint \"checkpoints/final state.chk\"\n"
                               "load_checkpoint \"checkpoints/final state.chk\"\n";
    const grav_cmd::CommandParseResult result = grav_cmd::CommandParser::parseScript(script);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(result.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(result.requests.size(), 4u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(result.requests[0].name, "load_config");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::string>(result.requests[0].arguments[0]), "configs/test scene.ini");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(result.requests[1].name, "export_snapshot");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::string>(result.requests[1].arguments[0]), "exports/final state.vtk");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::string>(result.requests[1].arguments[1]), "vtk");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(result.requests[2].name, "save_checkpoint");
    EXPECT_EQ(std::get<std::string>(result.requests[2].arguments[0]),
              "checkpoints/final state.chk");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(result.requests[3].name, "load_checkpoint");
    EXPECT_EQ(std::get<std::string>(result.requests[3].arguments[0]),
              "checkpoints/final state.chk");
}
/// Description: Executes the TEST operation.
TEST(CommandParserTest, TST_UNT_MODCLI_007_RejectsUnknownCommandWithLineNumber)
{
    const grav_cmd::CommandParseResult result =
        /// Description: Executes the parseScript operation.
        grav_cmd::CommandParser::parseScript("help\nbogus\n");
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(result.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(result.error, "line 2: unknown command 'bogus'");
}
/// Description: Executes the TEST operation.
TEST(CommandParserTest, TST_UNT_MODCLI_008_RejectsWrongArityAndInvalidNumericValues)
{
    const grav_cmd::CommandParseResult arity =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("connect 127.0.0.1", 4u);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(arity.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(arity.error, "line 4: wrong arity for 'connect'");
    const grav_cmd::CommandParseResult integer =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("step nope", 6u);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(integer.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(integer.error, "line 6: invalid integer 'nope'");
    const grav_cmd::CommandParseResult floating =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("run_until soon", 9u);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(floating.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(floating.error, "line 9: invalid float 'soon'");
}
/// Description: Executes the TEST operation.
TEST(CommandParserTest, TST_UNT_MODCLI_028_ParseScriptKeepsTailLineWithoutTrailingNewline)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseScript operation.
        grav_cmd::CommandParser::parseScript("status\nstep 2");
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 2u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.requests[0].name, "status");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.requests[1].name, "step");
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests[1].arguments.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::uint64_t>(parsed.requests[1].arguments[0]), 2u);
}
/// Description: Executes the TEST operation.
TEST(CommandParserTest, TST_UNT_MODCLI_029_ParseLineKeepsHashInsideQuotedToken)
{
    const grav_cmd::CommandParseResult parsed = grav_cmd::CommandParser::parseLine(
        "load_config \"cfg#release.ini\" # trailing comment", 12u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests[0].arguments.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.requests[0].name, "load_config");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::string>(parsed.requests[0].arguments[0]), "cfg#release.ini");
}
/// Description: Executes the TEST operation.
TEST(CommandParserTest, TST_UNT_MODCLI_030_ParseScriptStopsAtFirstInvalidLine)
{
    const std::string script = "status\n"
                               "run_until 3.0\n"
                               "run_steps nope\n"
                               "help\n";
    const grav_cmd::CommandParseResult parsed = grav_cmd::CommandParser::parseScript(script);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(parsed.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.error, "line 3: invalid integer 'nope'");
}
/// Description: Executes the TEST operation.
TEST(CommandParserTest, TST_UNT_MODCLI_033_ParseLineAcceptsCommentOnlyAndWhitespace)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("   # only comment", 2u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(parsed.requests.empty());
}
/// Description: Executes the TEST operation.
TEST(CommandParserTest, TST_UNT_MODCLI_034_ParseLineSupportsSingleQuotedTokens)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("load_config 'configs/release build.ini'", 7u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests[0].arguments.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::string>(parsed.requests[0].arguments[0]), "configs/release build.ini");
}
/// Description: Executes the TEST operation.
TEST(CommandParserTest, TST_UNT_MODCLI_035_ParseLineRejectsIntegerWithTrailingGarbage)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("step 12ms", 21u);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(parsed.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.error, "line 21: invalid integer '12ms'");
}
/// Description: Executes the TEST operation.
TEST(CommandParserTest, TST_UNT_MODCLI_036_ParseLineRejectsFloatWithTrailingGarbage)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("run_until 1.5s", 22u);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(parsed.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.error, "line 22: invalid float '1.5s'");
}
} // namespace grav_test_module_cli_command_parser
