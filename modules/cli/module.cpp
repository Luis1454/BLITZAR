#include <cstddef>

#include "client/ClientModuleApi.hpp"
#include "client/ClientModuleBoundary.hpp"
#include "modules/cli/module_cli_commands.hpp"
#include "modules/cli/module_cli_lifecycle.hpp"
#include "modules/cli/module_cli_state.hpp"

extern "C" GRAVITY_CLIENT_MODULE_EXPORT_ATTR const grav_module::ClientModuleExportsV1 *gravity_client_module_v1()
{
    static const grav_module::ClientModuleExportsV1 exports{
        grav_module::kClientModuleApiVersionV1,
        "cli",
        [](const grav_module::ClientHostContextV1 *hostContext, void **outModuleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            return grav_module_cli::ModuleCliLifecycle::create(
                hostContext,
                grav_module::ClientModuleStateSlot(outModuleState),
                grav_client::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void *moduleState) {
            grav_module_cli::ModuleCliLifecycle::destroy(
                grav_module::ClientModuleOpaqueState::fromRawPointer(moduleState));
        },
        [](void *moduleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            return grav_module_cli::ModuleCliLifecycle::start(
                grav_module::ClientModuleOpaqueState::fromRawPointer(moduleState),
                grav_client::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void *moduleState) {
            grav_module_cli::ModuleCliLifecycle::stop(
                grav_module::ClientModuleOpaqueState::fromRawPointer(moduleState));
        },
        [](void *moduleState, const char *commandLine, bool *outKeepRunning, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            grav_module_cli::ModuleState *state = static_cast<grav_module_cli::ModuleState *>(moduleState);
            const grav_client::ErrorBufferView errorView(errorBuffer, errorBufferSize);
            if (state == nullptr) {
                errorView.write("module state is null");
                return false;
            }
            return grav_module_cli::ModuleCliCommands::handleCommand(
                *state,
                commandLine != nullptr ? std::string_view(commandLine) : std::string_view(),
                grav_module::ClientModuleCommandControl(outKeepRunning),
                errorView);
        }
    };
    return &exports;
}
