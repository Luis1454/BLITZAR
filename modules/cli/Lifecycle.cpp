/*
 * @file modules/cli/Lifecycle.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>

#include "client/diagnostics/ErrorBuffer.hpp"
#include "config/core/Config.hpp"
#include "modules/cli/Lifecycle.hpp"
#include "modules/cli/State.hpp"

namespace bltzr_module_cli {
namespace {

bool createState(const bltzr_module::HostContextV1* hostContext,
                 const bltzr_module::StateSlot& outModuleState,
                 const bltzr_client::ErrorBufferView& errorBuffer)
{
    try {
        if (!outModuleState.isAvailable()) {
            errorBuffer.write("outModuleState is null");
            return false;
        }
        std::unique_ptr<State> state = std::make_unique<State>();
        if (hostContext != nullptr && hostContext->configPath != nullptr) {
            state->session.configPath = hostContext->configPath;
        }
        state->session.config = SimulationConfig::loadOrCreate(state->session.configPath);
        return outModuleState.assign(bltzr_module::OpaqueState::fromRawPointer(state.release()));
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

void destroyState(bltzr_module::OpaqueState moduleState)
{
    try {
        State* state = static_cast<State*>(moduleState.rawPointer());
        if (state != nullptr) {
            std::unique_ptr<State> ownedState(state);
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

bool startState(bltzr_module::OpaqueState moduleState,
                const bltzr_client::ErrorBufferView& errorBuffer)
{
    try {
        State* state = static_cast<State*>(moduleState.rawPointer());
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

void stopState(bltzr_module::OpaqueState moduleState)
{
    try {
        State* state = static_cast<State*>(moduleState.rawPointer());
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
} // namespace

bool Lifecycle::create(const bltzr_module::HostContextV1* hostContext,
                       const bltzr_module::StateSlot& outModuleState,
                       const bltzr_client::ErrorBufferView& errorBuffer)
{
    return createState(hostContext, outModuleState, errorBuffer);
}

void Lifecycle::destroy(bltzr_module::OpaqueState moduleState)
{
    destroyState(moduleState);
}

bool Lifecycle::start(bltzr_module::OpaqueState moduleState,
                      const bltzr_client::ErrorBufferView& errorBuffer)
{
    return startState(moduleState, errorBuffer);
}

void Lifecycle::stop(bltzr_module::OpaqueState moduleState)
{
    stopState(moduleState);
}

} // namespace bltzr_module_cli
