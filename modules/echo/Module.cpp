/*
 * @file modules/echo/Module.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Echo module used to validate client module loading behavior.
 */

#include "client/module/Api.hpp"
#include "client/module/Boundary.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

/*
 * @brief Documents the trim operation contract.
 * @param input Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::string trim(const std::string& input)
{
    const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
                         return std::isspace(c) != 0;
                     }).base();
    if (begin >= end)
        return {};
    return std::string(begin, end);
}

/*
 * @brief Defines the echo state type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct EchoState {
    std::string configPath;
};

static bool createEchoState(const bltzr_module::HostContextV1* context,
                            const bltzr_module::StateSlot& outModuleState,
                            const bltzr_client::ErrorBufferView& errorBuffer)
{
    try {
        if (!outModuleState.isAvailable()) {
            errorBuffer.write("outModuleState is null");
            return false;
        }
        std::unique_ptr<EchoState> state = std::make_unique<EchoState>();
        state->configPath = (context != nullptr && context->configPath != nullptr)
                                ? context->configPath
                                : "simulation.ini";
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

static void destroyEchoState(bltzr_module::OpaqueState moduleState)
{
    try {
        std::unique_ptr<EchoState> state(static_cast<EchoState*>(moduleState.rawPointer()));
    }
    catch (const std::exception& ex) {
        std::cerr << "[module-echo] destroy error: " << ex.what() << "\n";
    }
    catch (...) {
        std::cerr << "[module-echo] destroy error: unknown\n";
    }
}

static bool startEchoState(bltzr_module::OpaqueState moduleState,
                           const bltzr_client::ErrorBufferView& errorBuffer)
{
    try {
        EchoState* state = static_cast<EchoState*>(moduleState.rawPointer());
        if (state == nullptr) {
            errorBuffer.write("module state is null");
            return false;
        }
        std::cout << "[module-echo] started (config=" << state->configPath << ")\n";
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

static bool handleEchoCommand(std::string_view commandLine,
                              const bltzr_module::CommandControl& commandControl,
                              const bltzr_client::ErrorBufferView& errorBuffer)
{
    try {
        commandControl.setContinue();
        const std::string line = trim(std::string(commandLine));
        if (line.empty()) {
            return true;
        }
        if (line == "quit" || line == "exit") {
            commandControl.requestStop();
            return true;
        }
        std::cout << "[module-echo] " << line << "\n";
        return true;
    }
    catch (const std::exception& ex) {
        errorBuffer.write(ex.what());
        return false;
    }
    catch (...) {
        errorBuffer.write("unknown module command error");
        return false;
    }
}

extern "C" BLITZAR_CLIENT_MODULE_EXPORT_ATTR const bltzr_module::ExportsV1*
BLITZAR_client_module_v1()
{
    static const bltzr_module::ExportsV1 exports{
        bltzr_module::kApiVersionV1,
        "echo",
        [](const bltzr_module::HostContextV1* context, void** outModuleState,
           char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return createEchoState(
                context, bltzr_module::StateSlot(outModuleState),
                bltzr_client::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void* moduleState) {
            destroyEchoState(bltzr_module::OpaqueState::fromRawPointer(moduleState));
        },
        [](void* moduleState, char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return startEchoState(
                bltzr_module::OpaqueState::fromRawPointer(moduleState),
                bltzr_client::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void*) {
            try {
                std::cout << "[module-echo] stopped\n";
            }
            catch (...) {
            }
        },
        [](void*, const char* commandLine, bool* outKeepRunning, char* errorBuffer,
           std::size_t errorBufferSize) -> bool {
            return handleEchoCommand(
                commandLine != nullptr ? std::string_view(commandLine) : std::string_view(),
                bltzr_module::CommandControl(outKeepRunning),
                bltzr_client::ErrorBufferView(errorBuffer, errorBufferSize));
        }};
    return &exports;
}
