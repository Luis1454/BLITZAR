// File: modules/cli/module_cli_server_ops.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#ifndef GRAVITY_MODULES_CLI_MODULE_CLI_SERVER_OPS_HPP_
#define GRAVITY_MODULES_CLI_MODULE_CLI_SERVER_OPS_HPP_
#include "client/ErrorBuffer.hpp"
#include "modules/cli/module_cli_state.hpp"
#include <string>
#include <vector>

namespace grav_module_cli {
/// Description: Defines the ModuleCliServerOps data or behavior contract.
class ModuleCliServerOps final {
public:
    /// Description: Describes the command status operation contract.
    static bool commandStatus(ModuleState& state, const grav_client::ErrorBufferView& errorBuffer);
    /// Description: Describes the command step operation contract.
    static bool commandStep(ModuleState& state, const std::vector<std::string>& tokens,
                            const grav_client::ErrorBufferView& errorBuffer);
    /// Description: Describes the connect operation contract.
    static bool connect(ModuleState& state, const std::vector<std::string>& tokens,
                        const grav_client::ErrorBufferView& errorBuffer);
    /// Description: Describes the reconnect operation contract.
    static bool reconnect(ModuleState& state, const grav_client::ErrorBufferView& errorBuffer);
    /// Description: Describes the send simple command operation contract.
    static bool sendSimpleCommand(ModuleState& state, const std::string& cmd,
                                  const grav_client::ErrorBufferView& errorBuffer);
};
} // namespace grav_module_cli
#endif // GRAVITY_MODULES_CLI_MODULE_CLI_SERVER_OPS_HPP_
