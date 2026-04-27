// File: tests/unit/module_cli/text_and_commands.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "client/ClientModuleBoundary.hpp"
#include "modules/cli/module_cli_commands.hpp"
#include "modules/cli/module_cli_state.hpp"
#include "modules/cli/module_cli_text.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>
namespace grav_test_module_cli_text_and_commands {
/// Description: Executes the TEST operation.
TEST(ModuleCliTextTest, TST_UNT_MODCLI_001_TrimAndSplitTokens)
{
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_module_cli::ModuleCliText::trim("  hello  "), "hello");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_module_cli::ModuleCliText::trim(""), "");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_module_cli::ModuleCliText::trim("   "), "");
    const std::vector<std::string> tokens = grav_module_cli::ModuleCliText::splitTokens("step  12");
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(tokens.size(), 2u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(tokens[0], "step");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(tokens[1], "12");
}
/// Description: Executes the TEST operation.
TEST(ModuleCliCommandsTest, TST_UNT_MODCLI_002_HelpAndQuitCommandBehavior)
{
    grav_module_cli::ModuleState state;
    char errorBuffer[128] = {};
    grav_module::ClientModuleCommandResult commandResult;
    EXPECT_TRUE(grav_module_cli::ModuleCliCommands::handleCommand(
        state, "help", grav_module::ClientModuleCommandControl(commandResult.rawKeepRunningFlag()),
        /// Description: Executes the ErrorBufferView operation.
        grav_client::ErrorBufferView(errorBuffer, sizeof(errorBuffer))));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(commandResult.keepRunning());
    EXPECT_TRUE(grav_module_cli::ModuleCliCommands::handleCommand(
        state, "quit", grav_module::ClientModuleCommandControl(commandResult.rawKeepRunningFlag()),
        /// Description: Executes the ErrorBufferView operation.
        grav_client::ErrorBufferView(errorBuffer, sizeof(errorBuffer))));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(commandResult.keepRunning());
}
/// Description: Executes the TEST operation.
TEST(ModuleCliCommandsTest, TST_UNT_MODCLI_003_UnknownCommandReturnsError)
{
    grav_module_cli::ModuleState state;
    char errorBuffer[128] = {};
    grav_module::ClientModuleCommandResult commandResult;
    EXPECT_FALSE(grav_module_cli::ModuleCliCommands::handleCommand(
        state, "unknown_command",
        grav_module::ClientModuleCommandControl(commandResult.rawKeepRunningFlag()),
        /// Description: Executes the ErrorBufferView operation.
        grav_client::ErrorBufferView(errorBuffer, sizeof(errorBuffer))));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::string(errorBuffer), "line 1: unknown command 'unknown_command'");
}
} // namespace grav_test_module_cli_text_and_commands
