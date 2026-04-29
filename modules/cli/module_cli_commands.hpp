/*
 * @file modules/cli/module_cli_commands.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#ifndef BLITZAR_MODULES_CLI_MODULE_CLI_COMMANDS_HPP_
#define BLITZAR_MODULES_CLI_MODULE_CLI_COMMANDS_HPP_
#include "client/ClientModuleBoundary.hpp"
#include "client/ErrorBuffer.hpp"
#include "modules/cli/module_cli_state.hpp"
#include <string_view>

namespace bltzr_module_cli {
class ModuleCliCommands final {
public:
    static void printHelp();
    static bool handleCommand(ModuleState& state, std::string_view commandLine,
                              const bltzr_module::ClientModuleCommandControl& commandControl,
                              const bltzr_client::ErrorBufferView& errorBuffer);
};
} // namespace bltzr_module_cli
#endif // BLITZAR_MODULES_CLI_MODULE_CLI_COMMANDS_HPP_
