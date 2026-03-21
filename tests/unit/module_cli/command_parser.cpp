#include "command/CommandParser.hpp"

#include <gtest/gtest.h>

#include <string>

namespace grav_test_module_cli_command_parser {

TEST(CommandParserTest, TST_UNT_MODCLI_006_ParsesQuotedPathsAndComments)
{
    const std::string script =
        "  # heading\n"
        "\n"
        "load_config \"configs/test scene.ini\" # keep\n"
        "export_snapshot \"exports/final state.vtk\" vtk\n";

    const grav_cmd::CommandParseResult result = grav_cmd::CommandParser::parseScript(script);
    ASSERT_TRUE(result.ok);
    ASSERT_EQ(result.requests.size(), 2u);
    EXPECT_EQ(result.requests[0].name, "load_config");
    EXPECT_EQ(std::get<std::string>(result.requests[0].arguments[0]), "configs/test scene.ini");
    EXPECT_EQ(result.requests[1].name, "export_snapshot");
    EXPECT_EQ(std::get<std::string>(result.requests[1].arguments[0]), "exports/final state.vtk");
    EXPECT_EQ(std::get<std::string>(result.requests[1].arguments[1]), "vtk");
}

TEST(CommandParserTest, TST_UNT_MODCLI_007_RejectsUnknownCommandWithLineNumber)
{
    const grav_cmd::CommandParseResult result = grav_cmd::CommandParser::parseScript("help\nbogus\n");
    ASSERT_FALSE(result.ok);
    EXPECT_EQ(result.error, "line 2: unknown command 'bogus'");
}

TEST(CommandParserTest, TST_UNT_MODCLI_008_RejectsWrongArityAndInvalidNumericValues)
{
    const grav_cmd::CommandParseResult arity = grav_cmd::CommandParser::parseLine("connect 127.0.0.1", 4u);
    ASSERT_FALSE(arity.ok);
    EXPECT_EQ(arity.error, "line 4: wrong arity for 'connect'");

    const grav_cmd::CommandParseResult integer = grav_cmd::CommandParser::parseLine("step nope", 6u);
    ASSERT_FALSE(integer.ok);
    EXPECT_EQ(integer.error, "line 6: invalid integer 'nope'");

    const grav_cmd::CommandParseResult floating = grav_cmd::CommandParser::parseLine("run_until soon", 9u);
    ASSERT_FALSE(floating.ok);
    EXPECT_EQ(floating.error, "line 9: invalid float 'soon'");
}

} // namespace grav_test_module_cli_command_parser
