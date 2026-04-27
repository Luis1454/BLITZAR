// File: modules/cli/module_cli_state.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#ifndef GRAVITY_MODULES_CLI_MODULE_CLI_STATE_HPP_
#define GRAVITY_MODULES_CLI_MODULE_CLI_STATE_HPP_
#include "command/CommandContext.hpp"
#include "command/CommandTransport.hpp"
namespace grav_module_cli {
/// Description: Defines the ModuleState data or behavior contract.
struct ModuleState {
    /// Description: Executes the ModuleState operation.
    ModuleState();
    grav_cmd::ServerClientCommandTransport transport;
    grav_cmd::CommandSessionState session;
};
} // namespace grav_module_cli
#endif // GRAVITY_MODULES_CLI_MODULE_CLI_STATE_HPP_
