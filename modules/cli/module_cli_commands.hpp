// File: modules/cli/module_cli_commands.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#ifndef GRAVITY_MODULES_CLI_MODULE_CLI_COMMANDS_HPP_
#define GRAVITY_MODULES_CLI_MODULE_CLI_COMMANDS_HPP_
#include "client/ClientModuleBoundary.hpp"
#include "client/ErrorBuffer.hpp"
#include "modules/cli/module_cli_state.hpp"
#include <string_view>
namespace grav_module_cli {
class ModuleCliCommands final {
public:
    static void printHelp();
    static bool handleCommand(ModuleState& state, std::string_view commandLine,
                              const grav_module::ClientModuleCommandControl& commandControl,
                              const grav_client::ErrorBufferView& errorBuffer);
};
} // namespace grav_module_cli
#endif // GRAVITY_MODULES_CLI_MODULE_CLI_COMMANDS_HPP_
