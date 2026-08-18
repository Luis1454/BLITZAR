/*
 * @file modules/cli/commands/CliCommands.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#ifndef BLITZAR_MODULES_CLI_COMMANDS_HPP_
#define BLITZAR_MODULES_CLI_COMMANDS_HPP_
#include "client/module/CliBoundary.hpp"
#include "client/diagnostics/CliErrorBuffer.hpp"
#include "modules/cli/state/CliState.hpp"
#include <string_view>

namespace bltzr_module_cli {
class Commands final {
public:
    static void printHelp();
    static bool handleCommand(State& state, std::string_view commandLine,
                              const bltzr_module::CommandControl& commandControl,
                              const bltzr_client::ErrorBufferView& errorBuffer);
};
} // namespace bltzr_module_cli
#endif // BLITZAR_MODULES_CLI_COMMANDS_HPP_
