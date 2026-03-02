#include "frontend/ErrorBuffer.hpp"
#include "frontend/FrontendModuleApi.hpp"
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
#include <vector>

std::string trim(const std::string &input)
{
    const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::vector<std::string> splitTokens(const std::string &line)
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

struct GuiProxyState {
    std::string configPath = "simulation.ini";
    std::string host = "127.0.0.1";
    std::uint16_t port = 4545u;
    bool autoStartBackend = false;
    grav_platform::ProcessHandle frontendProcess;
};

bool isRunning(const GuiProxyState &state)
{
    return state.frontendProcess.isRunning();
}

bool stopProcess(GuiProxyState &state, char *errorBuffer, std::size_t errorBufferSize)
{
    std::string processError;
    if (!state.frontendProcess.terminate(2000u, processError)) {
        grav_frontend::writeErrorBuffer(
            errorBuffer,
            errorBufferSize,
            processError.empty() ? "failed to terminate frontend process" : processError);
        return false;
    }
    return true;
}

std::vector<std::string> frontendLaunchArgs(const GuiProxyState &state)
{
    return {
        "--config",
        state.configPath,
        "--backend-host",
        state.host,
        "--backend-port",
        std::to_string(state.port),
        "--backend-autostart",
        state.autoStartBackend ? "true" : "false"
    };
}

bool launchProcess(
    GuiProxyState &state,
    const std::string &frontendExecutable,
    char *errorBuffer,
    std::size_t errorBufferSize)
{
    if (!stopProcess(state, errorBuffer, errorBufferSize)) {
        return false;
    }
    std::string processError;
    if (!state.frontendProcess.launch(
            frontendExecutable,
            frontendLaunchArgs(state),
            true,
            processError)) {
        grav_frontend::writeErrorBuffer(
            errorBuffer,
            errorBufferSize,
            processError.empty() ? "failed to launch frontend process" : processError);
        state.frontendProcess.clear();
        return false;
    }
    return true;
}

std::string runningCommandLine(const GuiProxyState &state)
{
    return state.frontendProcess.commandLine();
}

std::string runningPidLabel(const GuiProxyState &state)
{
    if (!state.frontendProcess.isRunning()) {
        return {};
    }
    return state.frontendProcess.pidString();
}

bool parseUnsignedPort(std::string_view raw, std::uint16_t &outPort)
{
    const std::string trimmed = trim(std::string(raw));
    if (trimmed.empty()) {
        return false;
    }
    std::size_t parsedCount = 0u;
    unsigned long parsedPort = 0ul;
    try {
        parsedPort = std::stoul(trimmed, &parsedCount, 10);
    } catch (...) {
        return false;
    }
    if (parsedCount != trimmed.size()
        || parsedPort == 0ul
        || parsedPort > static_cast<unsigned long>(std::numeric_limits<std::uint16_t>::max())) {
        return false;
    }
    outPort = static_cast<std::uint16_t>(parsedPort);
    return true;
}

void printHelp()
{
    std::cout
        << "[module-gui-proxy] commands:\n"
        << "  help\n"
        << "  status\n"
        << "  set-endpoint <host> <port>\n"
        << "  set-autostart <true|false>\n"
        << "  launch <frontend_executable_path>\n"
        << "  stop\n";
}

bool parseBool(std::string_view raw, bool &out)
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
GRAVITY_FRONTEND_MODULE_EXPORT const grav_module::FrontendModuleExportsV1 *gravity_frontend_module_v1()
{
    static const grav_module::FrontendModuleExportsV1 exports{
        grav_module::kFrontendModuleApiVersionV1,
        "gui-proxy-module",
        [](const grav_module::FrontendModuleHostContextV1 *context, void **outModuleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            try {
                if (outModuleState == nullptr) {
                    grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "outModuleState is null");
                    return false;
                }
                std::unique_ptr<GuiProxyState> state = std::make_unique<GuiProxyState>();
                if (context != nullptr && context->configPath != nullptr) {
                    state->configPath = context->configPath;
                }
                *outModuleState = state.release();
                return true;
            } catch (const std::exception &ex) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
                return false;
            } catch (...) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module create error");
                return false;
            }
        },
        [](void *moduleState) {
            try {
                std::unique_ptr<GuiProxyState> state(static_cast<GuiProxyState *>(moduleState));
                if (state != nullptr) {
                    std::array<char, 32> ignored{};
                    (void)stopProcess(*state, ignored.data(), ignored.size());
                }
            } catch (const std::exception &ex) {
                std::cerr << "[module-gui-proxy] destroy error: " << ex.what() << "\n";
            } catch (...) {
                std::cerr << "[module-gui-proxy] destroy error: unknown\n";
            }
        },
        [](void *moduleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            try {
                auto *state = static_cast<GuiProxyState *>(moduleState);
                if (state == nullptr) {
                    grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "module state is null");
                    return false;
                }
                std::cout << "[module-gui-proxy] ready (config=" << state->configPath
                          << ", endpoint=" << state->host << ":" << state->port
                          << ", autostart=" << (state->autoStartBackend ? "on" : "off")
                          << ")\n";
                return true;
            } catch (const std::exception &ex) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
                return false;
            } catch (...) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module start error");
                return false;
            }
        },
        [](void *moduleState) {
            try {
                auto *state = static_cast<GuiProxyState *>(moduleState);
                if (state != nullptr) {
                    std::array<char, 32> ignored{};
                    (void)stopProcess(*state, ignored.data(), ignored.size());
                }
            } catch (const std::exception &ex) {
                std::cerr << "[module-gui-proxy] stop error: " << ex.what() << "\n";
            } catch (...) {
                std::cerr << "[module-gui-proxy] stop error: unknown\n";
            }
        },
        [](void *moduleState, const char *commandLine, bool *outKeepRunning, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            try {
                auto *state = static_cast<GuiProxyState *>(moduleState);
                if (state == nullptr) {
                    grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "module state is null");
                    return false;
                }
                if (outKeepRunning != nullptr) {
                    *outKeepRunning = true;
                }
                const std::string line = trim(commandLine != nullptr ? std::string(commandLine) : std::string());
                if (line.empty()) {
                    return true;
                }
                const std::vector<std::string> tokens = splitTokens(line);
                if (tokens.empty()) {
                    return true;
                }
                const std::string &cmd = tokens[0];
                if (cmd == "help") {
                    printHelp();
                    return true;
                }
                if (cmd == "quit" || cmd == "exit") {
                    if (outKeepRunning != nullptr) {
                        *outKeepRunning = false;
                    }
                    return true;
                }
                if (cmd == "status") {
                    const bool running = isRunning(*state);
                    std::cout << "[module-gui-proxy] endpoint=" << state->host << ":" << state->port
                              << " autostart=" << (state->autoStartBackend ? "on" : "off")
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
                        grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "usage: set-endpoint <host> <port>");
                        return false;
                    }
                    std::uint16_t parsedPort = 0u;
                    if (!parseUnsignedPort(tokens[2], parsedPort)) {
                        grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "invalid port");
                        return false;
                    }
                    state->host = tokens[1];
                    state->port = parsedPort;
                    std::cout << "[module-gui-proxy] endpoint set to " << state->host << ":" << state->port << "\n";
                    return true;
                }
                if (cmd == "set-autostart") {
                    if (tokens.size() < 2u) {
                        grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "usage: set-autostart <true|false>");
                        return false;
                    }
                    bool value = false;
                    if (!parseBool(tokens[1], value)) {
                        grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "invalid bool value");
                        return false;
                    }
                    state->autoStartBackend = value;
                    std::cout << "[module-gui-proxy] autostart=" << (value ? "on" : "off") << "\n";
                    return true;
                }
                if (cmd == "launch") {
                    if (tokens.size() < 2u) {
                        grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "usage: launch <frontend_executable_path>");
                        return false;
                    }
                    if (!launchProcess(*state, tokens[1], errorBuffer, errorBufferSize)) {
                        return false;
                    }
                    std::cout << "[module-gui-proxy] launched: "
                              << state->frontendProcess.commandLine() << "\n";
                    return true;
                }
                if (cmd == "stop") {
                    if (!stopProcess(*state, errorBuffer, errorBufferSize)) {
                        return false;
                    }
                    std::cout << "[module-gui-proxy] frontend stopped\n";
                    return true;
                }

                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module command");
                return false;
            } catch (const std::exception &ex) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
                return false;
            } catch (...) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module command error");
                return false;
            }
        }
    };
    return &exports;
}
