// File: modules/cli/module_cli_lifecycle.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>

#include "client/ErrorBuffer.hpp"
#include "config/SimulationConfig.hpp"
#include "modules/cli/module_cli_lifecycle.hpp"
#include "modules/cli/module_cli_state.hpp"

namespace grav_module_cli {

class ModuleCliLifecycleLocal final {
public:
    static bool create(const grav_module::ClientHostContextV1* hostContext,
                       const grav_module::ClientModuleStateSlot& outModuleState,
                       const grav_client::ErrorBufferView& errorBuffer)
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
                grav_module::ClientModuleOpaqueState::fromRawPointer(state.release()));
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

    static void destroy(grav_module::ClientModuleOpaqueState moduleState)
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

    static bool start(grav_module::ClientModuleOpaqueState moduleState,
                      const grav_client::ErrorBufferView& errorBuffer)
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

    static void stop(grav_module::ClientModuleOpaqueState moduleState)
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

bool ModuleCliLifecycle::create(const grav_module::ClientHostContextV1* hostContext,
                                const grav_module::ClientModuleStateSlot& outModuleState,
                                const grav_client::ErrorBufferView& errorBuffer)
{
    return ModuleCliLifecycleLocal::create(hostContext, outModuleState, errorBuffer);
}

void ModuleCliLifecycle::destroy(grav_module::ClientModuleOpaqueState moduleState)
{
    ModuleCliLifecycleLocal::destroy(moduleState);
}

bool ModuleCliLifecycle::start(grav_module::ClientModuleOpaqueState moduleState,
                               const grav_client::ErrorBufferView& errorBuffer)
{
    return ModuleCliLifecycleLocal::start(moduleState, errorBuffer);
}

void ModuleCliLifecycle::stop(grav_module::ClientModuleOpaqueState moduleState)
{
    ModuleCliLifecycleLocal::stop(moduleState);
}

} // namespace grav_module_cli
