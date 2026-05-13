/*
 * @file tests/unit/module_cli/text_and_commands.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "client/module/Boundary.hpp"
#include "modules/cli/Commands.hpp"
#include "modules/cli/State.hpp"
#include "modules/cli/Text.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace bltzr_test_module_cli_text_and_commands {
TEST(ModuleCliTextTest, TST_UNT_MODCLI_001_TrimAndSplitTokens)
{
    EXPECT_EQ(bltzr_module_cli::trim("  hello  "), "hello");
    EXPECT_EQ(bltzr_module_cli::trim(""), "");
    EXPECT_EQ(bltzr_module_cli::trim("   "), "");
    const std::vector<std::string> tokens =
        bltzr_module_cli::splitTokens("step  12");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0], "step");
    EXPECT_EQ(tokens[1], "12");
}

TEST(ModuleCliCommandsTest, TST_UNT_MODCLI_002_HelpAndQuitCommandBehavior)
{
    bltzr_module_cli::State state;
    char errorBuffer[128] = {};
    bltzr_module::Result commandResult;
    EXPECT_TRUE(bltzr_module_cli::Commands::handleCommand(
        state, "help", bltzr_module::CommandControl(commandResult.rawKeepRunningFlag()),
        bltzr_client::ErrorBufferView(errorBuffer, sizeof(errorBuffer))));
    EXPECT_TRUE(commandResult.keepRunning());
    EXPECT_TRUE(bltzr_module_cli::Commands::handleCommand(
        state, "quit", bltzr_module::CommandControl(commandResult.rawKeepRunningFlag()),
        bltzr_client::ErrorBufferView(errorBuffer, sizeof(errorBuffer))));
    EXPECT_FALSE(commandResult.keepRunning());
}

TEST(ModuleCliCommandsTest, TST_UNT_MODCLI_003_UnknownCommandReturnsError)
{
    bltzr_module_cli::State state;
    char errorBuffer[128] = {};
    bltzr_module::Result commandResult;
    EXPECT_FALSE(bltzr_module_cli::Commands::handleCommand(
        state, "unknown_command",
        bltzr_module::CommandControl(commandResult.rawKeepRunningFlag()),
        bltzr_client::ErrorBufferView(errorBuffer, sizeof(errorBuffer))));
    EXPECT_EQ(std::string(errorBuffer), "line 1: unknown command 'unknown_command'");
}
} // namespace bltzr_test_module_cli_text_and_commands
