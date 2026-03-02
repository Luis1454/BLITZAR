#include <cstddef>

#include "frontend/ErrorBuffer.hpp"
#include "frontend/FrontendModuleApi.hpp"
#include "modules/cli/module_cli_commands.hpp"
#include "modules/cli/module_cli_lifecycle.hpp"
#include "modules/cli/module_cli_state.hpp"

GRAVITY_FRONTEND_MODULE_EXPORT const grav_module::FrontendModuleExportsV1 *gravity_frontend_module_v1()
{
    static const grav_module::FrontendModuleExportsV1 exports{
        grav_module::kFrontendModuleApiVersionV1,
        "cli-module",
        [](const grav_module::FrontendModuleHostContextV1 *hostContext, void **outModuleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            return grav_module_cli::ModuleCliLifecycle::create(hostContext, outModuleState, errorBuffer, errorBufferSize);
        },
        [](void *moduleState) {
            grav_module_cli::ModuleCliLifecycle::destroy(moduleState);
        },
        [](void *moduleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            return grav_module_cli::ModuleCliLifecycle::start(moduleState, errorBuffer, errorBufferSize);
        },
        [](void *moduleState) {
            grav_module_cli::ModuleCliLifecycle::stop(moduleState);
        },
        [](void *moduleState, const char *commandLine, bool *outKeepRunning, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            grav_module_cli::ModuleState *state = static_cast<grav_module_cli::ModuleState *>(moduleState);
            if (state == nullptr) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "module state is null");
                return false;
            }
            return grav_module_cli::ModuleCliCommands::handleCommand(*state, commandLine, outKeepRunning, errorBuffer, errorBufferSize);
        }
    };
    return &exports;
}
