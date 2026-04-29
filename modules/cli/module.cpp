/*
 * @file modules/cli/module.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#include "client/ClientModuleBoundary.hpp"
#include "modules/cli/module_cli_commands.hpp"
#include "modules/cli/module_cli_lifecycle.hpp"
#include "modules/cli/module_cli_state.hpp"
extern "C" BLITZAR_CLIENT_MODULE_EXPORT_ATTR const bltzr_module::ClientModuleExportsV1*
#include "client/ClientModuleApi.hpp"
#include <cstddef>
BLITZAR_client_module_v1()
{
    static const bltzr_module::ClientModuleExportsV1 exports{
        bltzr_module::kClientModuleApiVersionV1,
        "cli",
        [](const bltzr_module::ClientHostContextV1* hostContext, void** outModuleState,
           char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return bltzr_module_cli::ModuleCliLifecycle::create(
                hostContext, bltzr_module::ClientModuleStateSlot(outModuleState),
                bltzr_client::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void* moduleState) {
            bltzr_module_cli::ModuleCliLifecycle::destroy(
                bltzr_module::ClientModuleOpaqueState::fromRawPointer(moduleState));
        },
        [](void* moduleState, char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return bltzr_module_cli::ModuleCliLifecycle::start(
                bltzr_module::ClientModuleOpaqueState::fromRawPointer(moduleState),
                bltzr_client::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void* moduleState) {
            bltzr_module_cli::ModuleCliLifecycle::stop(
                bltzr_module::ClientModuleOpaqueState::fromRawPointer(moduleState));
        },
        [](void* moduleState, const char* commandLine, bool* outKeepRunning, char* errorBuffer,
           std::size_t errorBufferSize) -> bool {
            bltzr_module_cli::ModuleState* state =
                static_cast<bltzr_module_cli::ModuleState*>(moduleState);
            const bltzr_client::ErrorBufferView errorView(errorBuffer, errorBufferSize);
            if (state == nullptr) {
                errorView.write("module state is null");
                return false;
            }
            return bltzr_module_cli::ModuleCliCommands::handleCommand(
                *state, commandLine != nullptr ? std::string_view(commandLine) : std::string_view(),
                bltzr_module::ClientModuleCommandControl(outKeepRunning), errorView);
        }};
    return &exports;
}
