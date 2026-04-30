/*
 * @file modules/proxy/module.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Proxy module used to validate runtime forwarding behavior.
 */

#include "client/ClientModuleApi.hpp"
#include "client/ClientModuleBoundary.hpp"
#include "platform/PlatformProcess.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

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
 * @brief Documents the split tokens operation contract.
 * @param line Input value used by this contract.
 * @return std::vector<std::string> value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::vector<std::string> splitTokens(const std::string& line)
{
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    char quoteChar = '\0';
    for (char c : line) {
        if (!inQuotes && (c == '"' || c == '\'')) {
            inQuotes = true;
            quoteChar = c;
            continue;
        }
        if (inQuotes && c == quoteChar) {
            inQuotes = false;
            quoteChar = '\0';
            continue;
        }
        if (!inQuotes && std::isspace(static_cast<unsigned char>(c)) != 0) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(c);
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

/*
 * @brief Documents the proxy error operation contract.
 * @param operation Input value used by this contract.
 * @param detail Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::string proxyError(std::string_view operation, std::string_view detail)
{
    return std::string("module-gui-proxy ") + std::string(operation) + ": " + std::string(detail);
}

/*
 * @brief Documents the write proxy error operation contract.
 * @param errorBuffer Input value used by this contract.
 * @param operation Input value used by this contract.
 * @param detail Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static void writeProxyError(const bltzr_client::ErrorBufferView& errorBuffer,
                            std::string_view operation, std::string_view detail)
{
    errorBuffer.write(proxyError(operation, detail));
}

/*
 * @brief Defines the gui proxy state type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct GuiProxyState {
    std::string configPath = "simulation.ini";
    std::string host = "127.0.0.1";
    std::uint16_t port = 4545u;
    bool autoStartServer = false;
    bltzr_platform::ProcessHandle clientProcess;
};

/*
 * @brief Documents the is running operation contract.
 * @param state Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static bool isRunning(const GuiProxyState& state)
{
    return state.clientProcess.isRunning();
}

/*
 * @brief Documents the stop process operation contract.
 * @param state Input value used by this contract.
 * @param errorBuffer Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static bool stopProcess(GuiProxyState& state, const bltzr_client::ErrorBufferView& errorBuffer)
{
    std::string processError;
    if (!state.clientProcess.terminate(2000u, processError)) {
        errorBuffer.write(processError.empty() ? "failed to terminate client process"
                                               : processError);
        return false;
    }
    return true;
}

/*
 * @brief Documents the client launch args operation contract.
 * @param state Input value used by this contract.
 * @return std::vector<std::string> value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::vector<std::string> clientLaunchArgs(const GuiProxyState& state)
{
    return {"--config",           state.configPath,
            "--server-host",      state.host,
            "--server-port",      std::to_string(state.port),
            "--server-autostart", state.autoStartServer ? "true" : "false"};
}

/*
 * @brief Documents the launch process operation contract.
 * @param state Input value used by this contract.
 * @param clientExecutable Input value used by this contract.
 * @param errorBuffer Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static bool launchProcess(GuiProxyState& state, const std::string& clientExecutable,
                          const bltzr_client::ErrorBufferView& errorBuffer)
{
    if (!stopProcess(state, errorBuffer)) {
        return false;
    }
    std::string processError;
    if (!state.clientProcess.launch(clientExecutable, clientLaunchArgs(state), true,
                                    processError)) {
        errorBuffer.write(processError.empty() ? "failed to launch client process" : processError);
        state.clientProcess.clear();
        return false;
    }
    return true;
}

/*
 * @brief Documents the running command line operation contract.
 * @param state Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::string runningCommandLine(const GuiProxyState& state)
{
    return state.clientProcess.commandLine();
}

/*
 * @brief Documents the running pid label operation contract.
 * @param state Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::string runningPidLabel(const GuiProxyState& state)
{
    if (!state.clientProcess.isRunning()) {
        return {};
    }
    return state.clientProcess.pidString();
}

/*
 * @brief Documents the parse unsigned port operation contract.
 * @param raw Input value used by this contract.
 * @param outPort Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static bool parseUnsignedPort(std::string_view raw, std::uint16_t& outPort)
{
    const std::string trimmed = trim(std::string(raw));
    if (trimmed.empty()) {
        return false;
    }
    std::size_t parsedCount = 0u;
    unsigned long parsedPort = 0ul;
    try {
        parsedPort = std::stoul(trimmed, &parsedCount, 10);
    }
    catch (const std::exception&) {
        return false;
    }
    if (parsedCount != trimmed.size() || parsedPort == 0ul ||
        parsedPort > static_cast<unsigned long>(std::numeric_limits<std::uint16_t>::max())) {
        return false;
    }
    outPort = static_cast<std::uint16_t>(parsedPort);
    return true;
}

/*
 * @brief Documents the print help operation contract.
 * @param None This contract does not take explicit parameters.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static void printHelp()
{
    std::cout << "[module-gui-proxy] commands:\n"
              << "  help\n"
              << "  status\n"
              << "  set-endpoint <host> <port>\n"
              << "  set-autostart <true|false>\n"
              << "  launch <client_executable_path>\n"
              << "  stop\n";
}

/*
 * @brief Documents the parse bool operation contract.
 * @param raw Input value used by this contract.
 * @param out Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static bool parseBool(std::string_view raw, bool& out)
{
    std::string value(raw);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (value == "1" || value == "true" || value == "on" || value == "yes") {
        out = true;
        return true;
    }
    if (value == "0" || value == "false" || value == "off" || value == "no") {
        out = false;
        return true;
    }
    return false;
}

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
    static bool create(const bltzr_module::ClientHostContextV1* context,
                       const bltzr_module::ClientModuleStateSlot& outModuleState,
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
                bltzr_module::ClientModuleOpaqueState::fromRawPointer(state.release()));
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
    static GuiProxyState* requireState(bltzr_module::ClientModuleOpaqueState moduleState,
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
    static void destroy(bltzr_module::ClientModuleOpaqueState moduleState)
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
    static bool start(bltzr_module::ClientModuleOpaqueState moduleState,
                      const bltzr_client::ErrorBufferView& errorBuffer)
    {
        try {
            GuiProxyState* state = requireState(moduleState, errorBuffer);
            if (state == nullptr)
                return false;
            std::cout << "[module-gui-proxy] ready (config=" << state->configPath
                      << ", endpoint=" << state->host << ":" << state->port
                      << ", autostart=" << (state->autoStartServer ? "on" : "off") << ")\n";
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
    static void stop(bltzr_module::ClientModuleOpaqueState moduleState)
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

extern "C" BLITZAR_CLIENT_MODULE_EXPORT_ATTR const bltzr_module::ClientModuleExportsV1*
BLITZAR_client_module_v1()
{
    static const bltzr_module::ClientModuleExportsV1 exports{
        bltzr_module::kClientModuleApiVersionV1,
        "gui",
        [](const bltzr_module::ClientHostContextV1* context, void** outModuleState,
           char* errorBuffer, std::size_t errorBufferSize) -> bool {
            return GuiProxyModuleBoundary::create(
                context, bltzr_module::ClientModuleStateSlot(outModuleState),
                bltzr_client::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void* moduleState) {
            GuiProxyModuleBoundary::destroy(
                bltzr_module::ClientModuleOpaqueState::fromRawPointer(moduleState));
        },
        [](void* moduleState, char* errorBuffer, std::size_t errorBufferSize) -> bool {
            const bltzr_client::ErrorBufferView errorView(errorBuffer, errorBufferSize);
            try {
                return GuiProxyModuleBoundary::start(
                    bltzr_module::ClientModuleOpaqueState::fromRawPointer(moduleState), errorView);
            }
            catch (...) {
                writeProxyError(errorView, "start", "non-standard exception");
                return false;
            }
        },
        [](void* moduleState) {
            GuiProxyModuleBoundary::stop(
                bltzr_module::ClientModuleOpaqueState::fromRawPointer(moduleState));
        },
        [](void* moduleState, const char* commandLine, bool* outKeepRunning, char* errorBuffer,
           std::size_t errorBufferSize) -> bool {
            const bltzr_client::ErrorBufferView errorView(errorBuffer, errorBufferSize);
            const bltzr_module::ClientModuleCommandControl commandControl(outKeepRunning);
            try {
                GuiProxyState* state = GuiProxyModuleBoundary::requireState(
                    bltzr_module::ClientModuleOpaqueState::fromRawPointer(moduleState), errorView);
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
                              << " running=" << (running ? "yes" : "no");
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
