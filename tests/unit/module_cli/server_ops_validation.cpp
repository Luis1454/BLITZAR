// File: tests/unit/module_cli/server_ops_validation.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "client/ErrorBuffer.hpp"
#include "modules/cli/module_cli_server_ops.hpp"
#include "modules/cli/module_cli_state.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>
namespace grav_test_module_cli_server_ops {
/// Description: Executes the TEST operation.
TEST(ModuleCliServerOpsTest, TST_UNT_MODCLI_004_CommandStepRejectsInvalidCount)
{
    grav_module_cli::ModuleState state;
    char errorBuffer[128] = {};
    const std::vector<std::string> tokens = {"step", "0"};
    EXPECT_FALSE(grav_module_cli::ModuleCliServerOps::commandStep(
        /// Description: Executes the ErrorBufferView operation.
        state, tokens, grav_client::ErrorBufferView(errorBuffer, sizeof(errorBuffer))));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::string(errorBuffer), "invalid step count");
}
/// Description: Executes the TEST operation.
TEST(ModuleCliServerOpsTest, TST_UNT_MODCLI_005_ConnectRejectsInvalidPort)
{
    grav_module_cli::ModuleState state;
    char errorBuffer[128] = {};
    const std::vector<std::string> tokens = {"connect", "127.0.0.1", "70000"};
    EXPECT_FALSE(grav_module_cli::ModuleCliServerOps::connect(
        /// Description: Executes the ErrorBufferView operation.
        state, tokens, grav_client::ErrorBufferView(errorBuffer, sizeof(errorBuffer))));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::string(errorBuffer), "invalid server port");
}
} // namespace grav_test_module_cli_server_ops
