/*
 * @file tests/unit/module_cli/text_and_commands.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "client/ClientModuleBoundary.hpp"
#include "modules/cli/module_cli_commands.hpp"
#include "modules/cli/module_cli_state.hpp"
#include "modules/cli/module_cli_text.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace bltzr_test_module_cli_text_and_commands {
TEST(ModuleCliTextTest, TST_UNT_MODCLI_001_TrimAndSplitTokens)
{
    EXPECT_EQ(bltzr_module_cli::ModuleCliText::trim("  hello  "), "hello");
    EXPECT_EQ(bltzr_module_cli::ModuleCliText::trim(""), "");
    EXPECT_EQ(bltzr_module_cli::ModuleCliText::trim("   "), "");
    const std::vector<std::string> tokens =
        bltzr_module_cli::ModuleCliText::splitTokens("step  12");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0], "step");
    EXPECT_EQ(tokens[1], "12");
}

TEST(ModuleCliCommandsTest, TST_UNT_MODCLI_002_HelpAndQuitCommandBehavior)
{
    bltzr_module_cli::ModuleState state;
    char errorBuffer[128] = {};
    bltzr_module::ClientModuleCommandResult commandResult;
    EXPECT_TRUE(bltzr_module_cli::ModuleCliCommands::handleCommand(
        state, "help", bltzr_module::ClientModuleCommandControl(commandResult.rawKeepRunningFlag()),
        bltzr_client::ErrorBufferView(errorBuffer, sizeof(errorBuffer))));
    EXPECT_TRUE(commandResult.keepRunning());
    EXPECT_TRUE(bltzr_module_cli::ModuleCliCommands::handleCommand(
        state, "quit", bltzr_module::ClientModuleCommandControl(commandResult.rawKeepRunningFlag()),
        bltzr_client::ErrorBufferView(errorBuffer, sizeof(errorBuffer))));
    EXPECT_FALSE(commandResult.keepRunning());
}

TEST(ModuleCliCommandsTest, TST_UNT_MODCLI_003_UnknownCommandReturnsError)
{
    bltzr_module_cli::ModuleState state;
    char errorBuffer[128] = {};
    bltzr_module::ClientModuleCommandResult commandResult;
    EXPECT_FALSE(bltzr_module_cli::ModuleCliCommands::handleCommand(
        state, "unknown_command",
        bltzr_module::ClientModuleCommandControl(commandResult.rawKeepRunningFlag()),
        bltzr_client::ErrorBufferView(errorBuffer, sizeof(errorBuffer))));
    EXPECT_EQ(std::string(errorBuffer), "line 1: unknown command 'unknown_command'");
}
} // namespace bltzr_test_module_cli_text_and_commands
