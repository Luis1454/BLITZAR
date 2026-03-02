#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "modules/cli/module_cli_state.hpp"

namespace grav_module_cli {

class ModuleCliBackendOps final {
public:
    static bool commandStatus(ModuleState &state, char *errorBuffer, std::size_t errorBufferSize);
    static bool commandStep(
        ModuleState &state,
        const std::vector<std::string> &tokens,
        char *errorBuffer,
        std::size_t errorBufferSize);
    static bool connect(
        ModuleState &state,
        const std::vector<std::string> &tokens,
        char *errorBuffer,
        std::size_t errorBufferSize);
    static bool reconnect(ModuleState &state, char *errorBuffer, std::size_t errorBufferSize);
    static bool sendSimpleCommand(
        ModuleState &state,
        const std::string &cmd,
        char *errorBuffer,
        std::size_t errorBufferSize);
};

} // namespace grav_module_cli
