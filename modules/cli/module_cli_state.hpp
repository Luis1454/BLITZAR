/*
 * @file modules/cli/module_cli_state.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#ifndef GRAVITY_MODULES_CLI_MODULE_CLI_STATE_HPP_
#define GRAVITY_MODULES_CLI_MODULE_CLI_STATE_HPP_
#include "command/CommandContext.hpp"
#include "command/CommandTransport.hpp"

namespace grav_module_cli {
struct ModuleState {
    ModuleState();
    grav_cmd::ServerClientCommandTransport transport;
    grav_cmd::CommandSessionState session;
};
} // namespace grav_module_cli
#endif // GRAVITY_MODULES_CLI_MODULE_CLI_STATE_HPP_
