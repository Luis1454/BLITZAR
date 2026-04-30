/*
 * @file modules/cli/module_cli_lifecycle.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>

#include "client/ErrorBuffer.hpp"
#include "config/SimulationConfig.hpp"
#include "modules/cli/module_cli_lifecycle.hpp"
#include "modules/cli/module_cli_state.hpp"

namespace bltzr_module_cli {

class ModuleCliLifecycleLocal final {
public:
    static bool create(const bltzr_module::ClientHostContextV1* hostContext,
                       const bltzr_module::ClientModuleStateSlot& outModuleState,
                       const bltzr_client::ErrorBufferView& errorBuffer)
    {
        try {
            if (!outModuleState.isAvailable()) {
                errorBuffer.write("outModuleState is null");
                return false;
            }
            std::unique_ptr<ModuleState> state = std::make_unique<ModuleState>();
            if (hostContext != nullptr && hostContext->configPath != nullptr) {
                state->session.configPath = hostContext->configPath;
            }
            state->session.config = SimulationConfig::loadOrCreate(state->session.configPath);
            return outModuleState.assign(
                bltzr_module::ClientModuleOpaqueState::fromRawPointer(state.release()));
        }
        catch (const std::exception& ex) {
            errorBuffer.write(ex.what());
            return false;
        }
        catch (...) {
            errorBuffer.write("unknown module create error");
            return false;
        }
    }

    static void destroy(bltzr_module::ClientModuleOpaqueState moduleState)
    {
        try {
            ModuleState* state = static_cast<ModuleState*>(moduleState.rawPointer());
            if (state != nullptr) {
                std::unique_ptr<ModuleState> ownedState(state);
                ownedState->transport.disconnect();
            }
        }
        catch (const std::exception& ex) {
            std::cerr << "[module-cli] destroy error: " << ex.what() << "\n";
        }
        catch (...) {
            std::cerr << "[module-cli] destroy error: unknown\n";
        }
    }

    static bool start(bltzr_module::ClientModuleOpaqueState moduleState,
                      const bltzr_client::ErrorBufferView& errorBuffer)
    {
        try {
            ModuleState* state = static_cast<ModuleState*>(moduleState.rawPointer());
            if (state == nullptr) {
                errorBuffer.write("module state is null");
                return false;
            }
            if (!state->transport.connect(state->session.host, state->session.port)) {
                std::cout << "[module-cli] startup: server " << state->session.host << ":"
                          << state->session.port << " not reachable yet (retry on command)\n";
            }
            else {
                std::cout << "[module-cli] connected to " << state->session.host << ":"
                          << state->session.port << "\n";
            }
            return true;
        }
        catch (const std::exception& ex) {
            errorBuffer.write(ex.what());
            return false;
        }
        catch (...) {
            errorBuffer.write("unknown module start error");
            return false;
        }
    }

    static void stop(bltzr_module::ClientModuleOpaqueState moduleState)
    {
        try {
            ModuleState* state = static_cast<ModuleState*>(moduleState.rawPointer());
            if (state != nullptr) {
                state->transport.disconnect();
            }
        }
        catch (const std::exception& ex) {
            std::cerr << "[module-cli] stop error: " << ex.what() << "\n";
        }
        catch (...) {
            std::cerr << "[module-cli] stop error: unknown\n";
        }
    }
};

bool ModuleCliLifecycle::create(const bltzr_module::ClientHostContextV1* hostContext,
                                const bltzr_module::ClientModuleStateSlot& outModuleState,
                                const bltzr_client::ErrorBufferView& errorBuffer)
{
    return ModuleCliLifecycleLocal::create(hostContext, outModuleState, errorBuffer);
}

void ModuleCliLifecycle::destroy(bltzr_module::ClientModuleOpaqueState moduleState)
{
    ModuleCliLifecycleLocal::destroy(moduleState);
}

bool ModuleCliLifecycle::start(bltzr_module::ClientModuleOpaqueState moduleState,
                               const bltzr_client::ErrorBufferView& errorBuffer)
{
    return ModuleCliLifecycleLocal::start(moduleState, errorBuffer);
}

void ModuleCliLifecycle::stop(bltzr_module::ClientModuleOpaqueState moduleState)
{
    ModuleCliLifecycleLocal::stop(moduleState);
}

} // namespace bltzr_module_cli
