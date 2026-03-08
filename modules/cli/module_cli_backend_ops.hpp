#pragma once

#include <string>
#include <vector>

#include "frontend/ErrorBuffer.hpp"
#include "modules/cli/module_cli_state.hpp"

namespace grav_module_cli {

class ModuleCliBackendOps final {
public:
    static bool commandStatus(ModuleState &state, const grav_frontend::ErrorBufferView &errorBuffer);
    static bool commandStep(
        ModuleState &state,
        const std::vector<std::string> &tokens,
        const grav_frontend::ErrorBufferView &errorBuffer);
    static bool connect(
        ModuleState &state,
        const std::vector<std::string> &tokens,
        const grav_frontend::ErrorBufferView &errorBuffer);
    static bool reconnect(ModuleState &state, const grav_frontend::ErrorBufferView &errorBuffer);
    static bool sendSimpleCommand(
        ModuleState &state,
        const std::string &cmd,
        const grav_frontend::ErrorBufferView &errorBuffer);
};

} // namespace grav_module_cli
