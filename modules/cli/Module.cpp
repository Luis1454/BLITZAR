/*
 * @file modules/cli/Module.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#include "client/module/Api.hpp"
#include "client/module/Boundary.hpp"
#include "modules/cli/Commands.hpp"
#include "modules/cli/Lifecycle.hpp"
#include "modules/cli/State.hpp"
#include <cstddef>
#include <string_view>

extern "C" BLITZAR_CLIENT_MODULE_EXPORT_ATTR const bltzr_module::ExportsV1*
BLITZAR_client_module_v1()
{
    static const bltzr_module::ExportsV1 exports{
        bltzr_module::kApiVersionV1,
        "cli",
        [](const bltzr_module::HostContextV1* hostContext, void** outModuleState,
           char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return bltzr_module_cli::Lifecycle::create(
                hostContext, bltzr_module::StateSlot(outModuleState),
                bltzr_client::ErrorBufferView(errorBuffer, errorBufferSize));

        },
        [](void* moduleState) {
            bltzr_module_cli::Lifecycle::destroy(
                bltzr_module::OpaqueState::fromRawPointer(moduleState));

        },
        [](void* moduleState, char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return bltzr_module_cli::Lifecycle::start(
                bltzr_module::OpaqueState::fromRawPointer(moduleState),
                bltzr_client::ErrorBufferView(errorBuffer, errorBufferSize));

        },
        [](void* moduleState) {
            bltzr_module_cli::Lifecycle::stop(
                bltzr_module::OpaqueState::fromRawPointer(moduleState));

        },
        [](void* moduleState, const char* commandLine, bool* outKeepRunning, char* errorBuffer,
           std::size_t errorBufferSize) -> bool {

            bltzr_module_cli::State* state = static_cast<bltzr_module_cli::State*>(moduleState);
            const bltzr_client::ErrorBufferView errorView(errorBuffer, errorBufferSize);

            if (state == nullptr) {
                errorView.write("module state is null");
                return false;
            }

            return bltzr_module_cli::Commands::handleCommand(*state, commandLine != nullptr
                    ? std::string_view(commandLine)
                    : std::string_view(), bltzr_module::CommandControl(outKeepRunning), errorView);
        }};
    return &exports;
}
