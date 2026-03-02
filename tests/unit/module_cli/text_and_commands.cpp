#include "modules/cli/module_cli_commands.hpp"
#include "modules/cli/module_cli_state.hpp"
#include "modules/cli/module_cli_text.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace grav_test_module_cli_text_and_commands {

TEST(ModuleCliTextTest, TST_UNT_MODCLI_001_TrimAndSplitTokens)
{
    EXPECT_EQ(grav_module_cli::ModuleCliText::trim("  hello  "), "hello");
    EXPECT_EQ(grav_module_cli::ModuleCliText::trim(""), "");
    EXPECT_EQ(grav_module_cli::ModuleCliText::trim("   "), "");

    const std::vector<std::string> tokens = grav_module_cli::ModuleCliText::splitTokens("step  12");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0], "step");
    EXPECT_EQ(tokens[1], "12");
}

TEST(ModuleCliCommandsTest, TST_UNT_MODCLI_002_HelpAndQuitCommandBehavior)
{
    grav_module_cli::ModuleState state;
    bool keepRunning = true;
    char errorBuffer[128] = {};

    EXPECT_TRUE(grav_module_cli::ModuleCliCommands::handleCommand(
        state,
        "help",
        &keepRunning,
        errorBuffer,
        sizeof(errorBuffer)));
    EXPECT_TRUE(keepRunning);

    EXPECT_TRUE(grav_module_cli::ModuleCliCommands::handleCommand(
        state,
        "quit",
        &keepRunning,
        errorBuffer,
        sizeof(errorBuffer)));
    EXPECT_FALSE(keepRunning);
}

TEST(ModuleCliCommandsTest, TST_UNT_MODCLI_003_UnknownCommandReturnsError)
{
    grav_module_cli::ModuleState state;
    bool keepRunning = true;
    char errorBuffer[128] = {};

    EXPECT_FALSE(grav_module_cli::ModuleCliCommands::handleCommand(
        state,
        "unknown_command",
        &keepRunning,
        errorBuffer,
        sizeof(errorBuffer)));
    EXPECT_EQ(std::string(errorBuffer), "unknown module command");
}

} // namespace grav_test_module_cli_text_and_commands
