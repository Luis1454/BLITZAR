#include "client/ErrorBuffer.hpp"
#include "modules/cli/module_cli_server_ops.hpp"
#include "modules/cli/module_cli_state.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>
namespace grav_test_module_cli_server_ops {
TEST(ModuleCliServerOpsTest, TST_UNT_MODCLI_004_CommandStepRejectsInvalidCount)
{
    grav_module_cli::ModuleState state;
    char errorBuffer[128] = {};
    const std::vector<std::string> tokens = {"step", "0"};
    EXPECT_FALSE(grav_module_cli::ModuleCliServerOps::commandStep(
        state, tokens, grav_client::ErrorBufferView(errorBuffer, sizeof(errorBuffer))));
    EXPECT_EQ(std::string(errorBuffer), "invalid step count");
}
TEST(ModuleCliServerOpsTest, TST_UNT_MODCLI_005_ConnectRejectsInvalidPort)
{
    grav_module_cli::ModuleState state;
    char errorBuffer[128] = {};
    const std::vector<std::string> tokens = {"connect", "127.0.0.1", "70000"};
    EXPECT_FALSE(grav_module_cli::ModuleCliServerOps::connect(
        state, tokens, grav_client::ErrorBufferView(errorBuffer, sizeof(errorBuffer))));
    EXPECT_EQ(std::string(errorBuffer), "invalid server port");
}
} // namespace grav_test_module_cli_server_ops
