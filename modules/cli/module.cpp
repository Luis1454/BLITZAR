// File: modules/cli/module.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "client/ClientModuleBoundary.hpp"
#include "modules/cli/module_cli_commands.hpp"
#include "modules/cli/module_cli_lifecycle.hpp"
#include "modules/cli/module_cli_state.hpp"
extern "C" GRAVITY_CLIENT_MODULE_EXPORT_ATTR const grav_module::ClientModuleExportsV1*
#include "client/ClientModuleApi.hpp"
#include <cstddef>
/// Description: Executes the gravity_client_module_v1 operation.
gravity_client_module_v1()
{
    static const grav_module::ClientModuleExportsV1 exports{
        grav_module::kClientModuleApiVersionV1,
        "cli",
        [](const grav_module::ClientHostContextV1* hostContext, void** outModuleState,
           char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return grav_module_cli::ModuleCliLifecycle::create(
                hostContext, grav_module::ClientModuleStateSlot(outModuleState),
                /// Description: Executes the ErrorBufferView operation.
                grav_client::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void* moduleState) {
            grav_module_cli::ModuleCliLifecycle::destroy(
                /// Description: Executes the fromRawPointer operation.
                grav_module::ClientModuleOpaqueState::fromRawPointer(moduleState));
        },
        [](void* moduleState, char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return grav_module_cli::ModuleCliLifecycle::start(
                grav_module::ClientModuleOpaqueState::fromRawPointer(moduleState),
                /// Description: Executes the ErrorBufferView operation.
                grav_client::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void* moduleState) {
            grav_module_cli::ModuleCliLifecycle::stop(
                /// Description: Executes the fromRawPointer operation.
                grav_module::ClientModuleOpaqueState::fromRawPointer(moduleState));
        },
        [](void* moduleState, const char* commandLine, bool* outKeepRunning, char* errorBuffer,
           std::size_t errorBufferSize) -> bool {
            grav_module_cli::ModuleState* state =
                static_cast<grav_module_cli::ModuleState*>(moduleState);
            /// Description: Executes the errorView operation.
            const grav_client::ErrorBufferView errorView(errorBuffer, errorBufferSize);
            if (state == nullptr) {
                errorView.write("module state is null");
                return false;
            }
            return grav_module_cli::ModuleCliCommands::handleCommand(
                *state, commandLine != nullptr ? std::string_view(commandLine) : std::string_view(),
                /// Description: Executes the ClientModuleCommandControl operation.
                grav_module::ClientModuleCommandControl(outKeepRunning), errorView);
        }};
    return &exports;
}
