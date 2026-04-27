/*
 * @file modules/cli/module_cli_server_ops.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#ifndef GRAVITY_MODULES_CLI_MODULE_CLI_SERVER_OPS_HPP_
#define GRAVITY_MODULES_CLI_MODULE_CLI_SERVER_OPS_HPP_
#include "client/ErrorBuffer.hpp"
#include "modules/cli/module_cli_state.hpp"
#include <string>
#include <vector>

namespace grav_module_cli {
class ModuleCliServerOps final {
public:
    static bool commandStatus(ModuleState& state, const grav_client::ErrorBufferView& errorBuffer);
    static bool commandStep(ModuleState& state, const std::vector<std::string>& tokens,
                            const grav_client::ErrorBufferView& errorBuffer);
    static bool connect(ModuleState& state, const std::vector<std::string>& tokens,
                        const grav_client::ErrorBufferView& errorBuffer);
    static bool reconnect(ModuleState& state, const grav_client::ErrorBufferView& errorBuffer);
    static bool sendSimpleCommand(ModuleState& state, const std::string& cmd,
                                  const grav_client::ErrorBufferView& errorBuffer);
};
} // namespace grav_module_cli
#endif // GRAVITY_MODULES_CLI_MODULE_CLI_SERVER_OPS_HPP_
