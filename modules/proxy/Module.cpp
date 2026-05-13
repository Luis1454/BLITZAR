/*
 * @file modules/proxy/Module.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Proxy module used to validate runtime forwarding behavior.
 */

#include "client/module/Api.hpp"
#include "client/module/Boundary.hpp"
#include "modules/proxy/Support.hpp"
#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

/*
 * @brief Defines the gui proxy module boundary type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class GuiProxyModuleBoundary final {
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
    static bool create(const bltzr_module::HostContextV1* context,
                       const bltzr_module::StateSlot& outModuleState,
                       const bltzr_client::ErrorBufferView& errorBuffer)
    {
        try {
            if (!outModuleState.isAvailable()) {
                errorBuffer.write("outModuleState is null");
                return false;
            }
            std::unique_ptr<GuiProxyState> state = std::make_unique<GuiProxyState>();
            if (context != nullptr && context->configPath != nullptr) {
                state->configPath = context->configPath;
            }
            return outModuleState.assign(
                bltzr_module::OpaqueState::fromRawPointer(state.release()));
        }
        catch (const std::exception& ex) {
            writeProxyError(errorBuffer, "create", ex.what());
            return false;
        }
        catch (...) {
            writeProxyError(errorBuffer, "create", "non-standard exception");
            return false;
        }
    }

    /*
     * @brief Documents the require state operation contract.
     * @param moduleState Input value used by this contract.
     * @param errorBuffer Input value used by this contract.
     * @return GuiProxyState* value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static GuiProxyState* requireState(bltzr_module::OpaqueState moduleState,
                                       const bltzr_client::ErrorBufferView& errorBuffer)
    {
        GuiProxyState* state = static_cast<GuiProxyState*>(moduleState.rawPointer());
        if (state == nullptr) {
            errorBuffer.write("module state is null");
        }
        return state;
    }

    /*
     * @brief Documents the destroy operation contract.
     * @param moduleState Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static void destroy(bltzr_module::OpaqueState moduleState)
    {
        try {
            std::unique_ptr<GuiProxyState> state(
                static_cast<GuiProxyState*>(moduleState.rawPointer()));
            if (state != nullptr) {
                std::array<char, 32> ignored{};
                (void)stopProcess(*state,
                                  bltzr_client::ErrorBufferView(ignored.data(), ignored.size()));
            }
        }
        catch (const std::exception& ex) {
            std::cerr << proxyError("destroy", ex.what()) << "\n";
        }
        catch (...) {
            std::cerr << proxyError("destroy", "non-standard exception") << "\n";
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
    static bool start(bltzr_module::OpaqueState moduleState,
                      const bltzr_client::ErrorBufferView& errorBuffer)
    {
        try {
            GuiProxyState* state = requireState(moduleState, errorBuffer);
            if (state == nullptr)
                return false;
            std::cout << "[module-gui-proxy] ready (config=" << state->configPath
                      << ", endpoint=" << state->host << ":" << state->port
                      << ", autostart=" << (state->autoStartServer ? "on" : "off") << ")\n";
            if (!launchFallbackWindow(*state, errorBuffer)) {
                return false;
            }
            return true;
        }
        catch (const std::exception& ex) {
            writeProxyError(errorBuffer, "start", ex.what());
            return false;
        }
        catch (...) {
            writeProxyError(errorBuffer, "start", "non-standard exception");
            return false;
        }
    }

    /*
     * @brief Documents the stop operation contract.
     * @param moduleState Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static void stop(bltzr_module::OpaqueState moduleState)
    {
        try {
            GuiProxyState* state = static_cast<GuiProxyState*>(moduleState.rawPointer());
            if (state != nullptr) {
                std::array<char, 32> ignored{};
                (void)stopProcess(*state,
                                  bltzr_client::ErrorBufferView(ignored.data(), ignored.size()));
            }
        }
        catch (const std::exception& ex) {
            std::cerr << proxyError("stop", ex.what()) << "\n";
        }
        catch (...) {
            std::cerr << proxyError("stop", "non-standard exception") << "\n";
        }
    }
};

extern "C" BLITZAR_CLIENT_MODULE_EXPORT_ATTR const bltzr_module::ExportsV1*
BLITZAR_client_module_v1()
{
    static const bltzr_module::ExportsV1 exports{
        bltzr_module::kApiVersionV1,
        "gui",
        [](const bltzr_module::HostContextV1* context, void** outModuleState,
           char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return GuiProxyModuleBoundary::create(
                context, bltzr_module::StateSlot(outModuleState),
                bltzr_client::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void* moduleState) {
            GuiProxyModuleBoundary::destroy(
                bltzr_module::OpaqueState::fromRawPointer(moduleState));
        },
        [](void* moduleState, char* errorBuffer, std::size_t errorBufferSize) -> bool {
            const bltzr_client::ErrorBufferView errorView(errorBuffer, errorBufferSize);
            try {
                return GuiProxyModuleBoundary::start(
                    bltzr_module::OpaqueState::fromRawPointer(moduleState), errorView);
            }
            catch (...) {
                writeProxyError(errorView, "start", "non-standard exception");
                return false;
            }
        },
        [](void* moduleState) {
            GuiProxyModuleBoundary::stop(
                bltzr_module::OpaqueState::fromRawPointer(moduleState));
        },
        [](void* moduleState, const char* commandLine, bool* outKeepRunning, char* errorBuffer,
           std::size_t errorBufferSize) -> bool {
            const bltzr_client::ErrorBufferView errorView(errorBuffer, errorBufferSize);
            const bltzr_module::CommandControl commandControl(outKeepRunning);
            try {
                GuiProxyState* state = GuiProxyModuleBoundary::requireState(
                    bltzr_module::OpaqueState::fromRawPointer(moduleState), errorView);
                if (state == nullptr)
                    return false;
                commandControl.setContinue();
                const std::string line =
                    trim(commandLine != nullptr ? std::string(commandLine) : std::string());
                if (line.empty()) {
                    return true;
                }
                const std::vector<std::string> tokens = splitTokens(line);
                if (tokens.empty()) {
                    return true;
                }
                const std::string& cmd = tokens[0];
                if (cmd == "help") {
                    printHelp();
                    return true;
                }
                if (cmd == "quit" || cmd == "exit") {
                    commandControl.requestStop();
                    return true;
                }
                if (cmd == "status") {
                    const bool running = isRunning(*state);
                    std::cout << "[module-gui-proxy] endpoint=" << state->host << ":" << state->port
                              << " autostart=" << (state->autoStartServer ? "on" : "off")
                              << " running=" << (running ? "yes" : "no")
                              << " window=" << (isWindowRunning(*state) ? "yes" : "no");
                    const std::string pid = runningPidLabel(*state);
                    if (!pid.empty()) {
                        std::cout << " pid=" << pid;
                    }
                    const std::string activeCommandLine = runningCommandLine(*state);
                    if (!activeCommandLine.empty()) {
                        std::cout << " cmd=" << activeCommandLine;
                    }
                    std::cout << "\n";
                    return true;
                }
                if (cmd == "wait") {
                    while (isWindowRunning(*state) || isRunning(*state)) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    }
                    commandControl.requestStop();
                    return true;
                }
                if (cmd == "set-endpoint") {
                    if (tokens.size() < 3u) {
                        errorView.write("usage: set-endpoint <host> <port>");
                        return false;
                    }
                    std::uint16_t parsedPort = 0u;
                    if (!parseUnsignedPort(tokens[2], parsedPort)) {
                        errorView.write("invalid port");
                        return false;
                    }
                    state->host = tokens[1];
                    state->port = parsedPort;
                    std::cout << "[module-gui-proxy] endpoint set to " << state->host << ":"
                              << state->port << "\n";
                    if (!launchFallbackWindow(*state, errorView)) {
                        return false;
                    }
                    return true;
                }
                if (cmd == "set-autostart") {
                    if (tokens.size() < 2u) {
                        errorView.write("usage: set-autostart <true|false>");
                        return false;
                    }
                    bool value = false;
                    if (!parseBool(tokens[1], value)) {
                        errorView.write("invalid bool value");
                        return false;
                    }
                    state->autoStartServer = value;
                    std::cout << "[module-gui-proxy] autostart=" << (value ? "on" : "off") << "\n";
                    return true;
                }
                if (cmd == "launch") {
                    if (tokens.size() < 2u) {
                        errorView.write("usage: launch <client_executable_path>");
                        return false;
                    }
                    if (!launchProcess(*state, tokens[1], errorView)) {
                        return false;
                    }
                    std::cout << "[module-gui-proxy] launched: "
                              << state->clientProcess.commandLine() << "\n";
                    return true;
                }
                if (cmd == "stop") {
                    if (!stopProcess(*state, errorView)) {
                        return false;
                    }
                    std::cout << "[module-gui-proxy] client stopped\n";
                    return true;
                }
                if (cmd == "show") {
                    if (!launchFallbackWindow(*state, errorView)) {
                        return false;
                    }
                    std::cout << "[module-gui-proxy] fallback window opened\n";
                    return true;
                }
                errorView.write("unknown module command");
                return false;
            }
            catch (const std::exception& ex) {
                writeProxyError(errorView, "command", ex.what());
                return false;
            }
            catch (...) {
                writeProxyError(errorView, "command", "non-standard exception");
                return false;
            }
        }};
    return &exports;
}
