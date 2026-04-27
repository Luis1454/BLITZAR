// File: tests/unit/module_cli/command_parser_expansion_b.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "command/CommandParser.hpp"
#include <gtest/gtest.h>
#include <string>
namespace grav_test_module_cli_command_parser_expansion_b {
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionBTest, TST_UNT_MODCLI_052_ParseLineSupportsDoubleQuotedCheckpointPath)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("load_checkpoint \"checkpoints/final #1.chk\"", 1u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.front().arguments.size(), 1u);
    EXPECT_EQ(std::get<std::string>(parsed.requests.front().arguments[0]),
              "checkpoints/final #1.chk");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionBTest, TST_UNT_MODCLI_053_ParseScriptHandlesCrlfLineEndings)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseScript operation.
        grav_cmd::CommandParser::parseScript("status\r\nhelp\r\n");
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 2u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.requests[0].name, "status");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.requests[1].name, "help");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionBTest, TST_UNT_MODCLI_054_ParseLineTreatsCommandNamesAsCaseSensitive)
{
    const grav_cmd::CommandParseResult parsed = grav_cmd::CommandParser::parseLine("Help", 3u);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(parsed.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.error, "line 3: unknown command 'Help'");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionBTest, TST_UNT_MODCLI_055_ParseLineAcceptsSetSolverModeToken)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("set_solver octree_gpu", 4u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.front().arguments.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::string>(parsed.requests.front().arguments[0]), "octree_gpu");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionBTest, TST_UNT_MODCLI_056_ParseLineAcceptsSetProfileToken)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("set_profile stress", 5u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.front().arguments.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::string>(parsed.requests.front().arguments[0]), "stress");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionBTest, TST_UNT_MODCLI_057_ParseLineAcceptsZeroRunSteps)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("run_steps 0", 6u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.front().arguments.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::uint64_t>(parsed.requests.front().arguments[0]), 0u);
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionBTest, TST_UNT_MODCLI_058_ParseLineRejectsNegativeUnsignedValue)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("run_steps -1", 7u);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(parsed.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.error, "line 7: invalid integer '-1'");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionBTest, TST_UNT_MODCLI_059_ParseLineAcceptsUnterminatedQuoteAsSingleToken)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("load_config \"configs/default.ini", 8u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.front().arguments.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::string>(parsed.requests.front().arguments[0]), "configs/default.ini");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionBTest, TST_UNT_MODCLI_060_ParseLineKeepsCommentMarkerInsideQuotes)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("export_snapshot \"frames/#001.vtk\" vtk", 9u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.front().arguments.size(), 2u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::string>(parsed.requests.front().arguments[0]), "frames/#001.vtk");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::string>(parsed.requests.front().arguments[1]), "vtk");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionBTest, TST_UNT_MODCLI_061_ParseScriptAcceptsEmptyInput)
{
    const grav_cmd::CommandParseResult parsed = grav_cmd::CommandParser::parseScript("");
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(parsed.requests.empty());
}
} // namespace grav_test_module_cli_command_parser_expansion_b
