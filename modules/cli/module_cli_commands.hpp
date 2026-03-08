#pragma once

#include <string_view>

#include "frontend/FrontendModuleBoundary.hpp"
#include "modules/cli/module_cli_state.hpp"

namespace grav_module_cli {

class ModuleCliCommands final {
public:
    static void printHelp();
    static bool handleCommand(
        ModuleState &state,
        std::string_view commandLine,
        const grav_module::FrontendModuleCommandControl &commandControl,
        const grav_frontend::ErrorBufferView &errorBuffer);
};

} // namespace grav_module_cli
