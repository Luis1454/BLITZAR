/*
 * @file modules/echo/module.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Echo module used to validate client module loading behavior.
 */

#include "client/ClientModuleApi.hpp"
#include "client/ClientModuleBoundary.hpp"
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

/*
 * @brief Defines the echo module local type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class EchoModuleLocal final {
public:
    /*
     * @brief Documents the create operation contract.
     * @param context Input value used by this contract.
     * @param outModuleState Input value used by this contract.
     * @param errorBuffer Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static bool create(const bltzr_module::ClientHostContextV1* context,
                       const bltzr_module::ClientModuleStateSlot& outModuleState,
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

    /*
     * @brief Documents the destroy operation contract.
     * @param moduleState Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static void destroy(bltzr_module::ClientModuleOpaqueState moduleState)
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

    /*
     * @brief Documents the start operation contract.
     * @param moduleState Input value used by this contract.
     * @param errorBuffer Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static bool start(bltzr_module::ClientModuleOpaqueState moduleState,
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

    /*
     * @brief Documents the handle command operation contract.
     * @param commandLine Input value used by this contract.
     * @param commandControl Input value used by this contract.
     * @param errorBuffer Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static bool handleCommand(std::string_view commandLine,
                              const bltzr_module::ClientModuleCommandControl& commandControl,
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
};

extern "C" BLITZAR_CLIENT_MODULE_EXPORT_ATTR const bltzr_module::ClientModuleExportsV1*
BLITZAR_client_module_v1()
{
    static const bltzr_module::ClientModuleExportsV1 exports{
        bltzr_module::kClientModuleApiVersionV1,
        "echo",
        [](const bltzr_module::ClientHostContextV1* context, void** outModuleState,
           char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return EchoModuleLocal::create(
                context, bltzr_module::ClientModuleStateSlot(outModuleState),
                bltzr_client::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void* moduleState) {
            EchoModuleLocal::destroy(
                bltzr_module::ClientModuleOpaqueState::fromRawPointer(moduleState));
        },
        [](void* moduleState, char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return EchoModuleLocal::start(
                bltzr_module::ClientModuleOpaqueState::fromRawPointer(moduleState),
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
            return EchoModuleLocal::handleCommand(
                commandLine != nullptr ? std::string_view(commandLine) : std::string_view(),
                bltzr_module::ClientModuleCommandControl(outKeepRunning),
                bltzr_client::ErrorBufferView(errorBuffer, errorBufferSize));
        }};
    return &exports;
}
