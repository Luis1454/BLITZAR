/*
 * @file modules/cli/module_cli_state.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#ifndef BLITZAR_MODULES_CLI_MODULE_CLI_STATE_HPP_
#define BLITZAR_MODULES_CLI_MODULE_CLI_STATE_HPP_
#include "command/CommandContext.hpp"
#include "command/CommandTransport.hpp"

namespace bltzr_module_cli {
struct ModuleState {
    ModuleState();
    bltzr_cmd::ServerClientCommandTransport transport;
    bltzr_cmd::CommandSessionState session;
};
} // namespace bltzr_module_cli
#endif // BLITZAR_MODULES_CLI_MODULE_CLI_STATE_HPP_
