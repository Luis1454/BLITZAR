#pragma once

#include <cstddef>

#include "modules/cli/module_cli_state.hpp"

namespace grav_module_cli {

class ModuleCliCommands final {
public:
    static void printHelp();
    static bool handleCommand(
        ModuleState &state,
        const char *commandLine,
        bool *outKeepRunning,
        char *errorBuffer,
        std::size_t errorBufferSize);
};

} // namespace grav_module_cli
