/*
 * @file modules/cli/Commands.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#ifndef BLITZAR_MODULES_CLI_COMMANDS_HPP_
#define BLITZAR_MODULES_CLI_COMMANDS_HPP_
#include "client/module/Boundary.hpp"
#include "client/diagnostics/ErrorBuffer.hpp"
#include "modules/cli/State.hpp"
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
