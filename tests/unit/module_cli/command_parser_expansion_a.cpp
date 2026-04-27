// File: tests/unit/module_cli/command_parser_expansion_a.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "command/CommandParser.hpp"
#include <gtest/gtest.h>
#include <string>
namespace grav_test_module_cli_command_parser_expansion_a {
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionATest, TST_UNT_MODCLI_042_ParseLineTrimsLeadingAndTrailingWhitespace)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("   status   ", 1u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.requests.front().name, "status");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionATest, TST_UNT_MODCLI_043_ParseLineAcceptsOptionalStepCountOmitted)
{
    const grav_cmd::CommandParseResult parsed = grav_cmd::CommandParser::parseLine("step", 2u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.requests.front().name, "step");
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(parsed.requests.front().arguments.empty());
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionATest, TST_UNT_MODCLI_044_ParseLineRejectsTooManyArgumentsForStatus)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("status now", 3u);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(parsed.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.error, "line 3: wrong arity for 'status'");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionATest, TST_UNT_MODCLI_045_ParseLineRejectsInvalidPortToken)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("connect 127.0.0.1 45a", 4u);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(parsed.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.error, "line 4: invalid integer '45a'");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionATest, TST_UNT_MODCLI_046_ParseLineParsesConnectPortAsUint)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("connect 10.0.0.5 4545", 5u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.front().arguments.size(), 2u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::string>(parsed.requests.front().arguments[0]), "10.0.0.5");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::uint64_t>(parsed.requests.front().arguments[1]), 4545u);
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionATest, TST_UNT_MODCLI_047_ParseLineAcceptsScientificFloatForSetDt)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("set_dt 1e-3", 6u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.front().arguments.size(), 1u);
    /// Description: Executes the EXPECT_DOUBLE_EQ operation.
    EXPECT_DOUBLE_EQ(std::get<double>(parsed.requests.front().arguments[0]), 1e-3);
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionATest, TST_UNT_MODCLI_048_ParseLineRejectsMalformedFloatForSetDt)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("set_dt 0.5,", 7u);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(parsed.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.error, "line 7: invalid float '0.5,'");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionATest, TST_UNT_MODCLI_049_ParseLineAcceptsExportSnapshotWithoutFormat)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("export_snapshot outputs/frame", 8u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.front().arguments.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::get<std::string>(parsed.requests.front().arguments[0]), "outputs/frame");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionATest, TST_UNT_MODCLI_050_ParseLineRejectsExportSnapshotTooManyArguments)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("export_snapshot a.vtk vtk extra", 9u);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(parsed.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.error, "line 9: wrong arity for 'export_snapshot'");
}
/// Description: Executes the TEST operation.
TEST(CommandParserExpansionATest, TST_UNT_MODCLI_051_ParseLineSupportsSingleQuotedCheckpointPath)
{
    const grav_cmd::CommandParseResult parsed =
        /// Description: Executes the parseLine operation.
        grav_cmd::CommandParser::parseLine("save_checkpoint 'checkpoints/final state.chk'", 10u);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.size(), 1u);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(parsed.requests.front().arguments.size(), 1u);
    EXPECT_EQ(std::get<std::string>(parsed.requests.front().arguments[0]),
              "checkpoints/final state.chk");
}
} // namespace grav_test_module_cli_command_parser_expansion_a
