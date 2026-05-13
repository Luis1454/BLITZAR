/*
 * @file modules/proxy/Support.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Internal support helpers for the GUI proxy client module.
 */

#include "modules/proxy/Support.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <limits>

std::string trim(const std::string& input)
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

std::vector<std::string> splitTokens(const std::string& line)
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

std::string proxyError(std::string_view operation, std::string_view detail)
{
    return std::string("module-gui-proxy ") + std::string(operation) + ": " + std::string(detail);
}

void writeProxyError(const bltzr_client::ErrorBufferView& errorBuffer, std::string_view operation,
                     std::string_view detail)
{
    errorBuffer.write(proxyError(operation, detail));
}

bool isRunning(const GuiProxyState& state)
{
    return state.clientProcess.isRunning();
}

bool isWindowRunning(const GuiProxyState& state)
{
    return state.windowProcess.isRunning();
}

static bool hasGraphicalDisplay()
{
    const char* display = std::getenv("DISPLAY");
    const char* waylandDisplay = std::getenv("WAYLAND_DISPLAY");
    return (display != nullptr && display[0] != '\0') ||
           (waylandDisplay != nullptr && waylandDisplay[0] != '\0');
}

static std::vector<std::string> clientLaunchArgs(const GuiProxyState& state)
{
    return {"--config",           state.configPath,
            "--server-host",      state.host,
            "--server-port",      std::to_string(state.port),
            "--server-autostart", state.autoStartServer ? "true" : "false"};
}

bool stopProcess(GuiProxyState& state, const bltzr_client::ErrorBufferView& errorBuffer)
{
    std::string processError;
    if (state.windowProcess.isRunning() &&
        !state.windowProcess.terminate(2000u, processError)) {
        errorBuffer.write(processError.empty() ? "failed to terminate fallback window"
                                               : processError);
        return false;
    }
    processError.clear();
    if (!state.clientProcess.terminate(2000u, processError)) {
        errorBuffer.write(processError.empty() ? "failed to terminate client process"
                                               : processError);
        return false;
    }
    return true;
}

bool launchFallbackWindow(GuiProxyState& state, const bltzr_client::ErrorBufferView& errorBuffer)
{
    if (!hasGraphicalDisplay()) {
        std::cout << "[module-gui-proxy] no graphical display; fallback window disabled\n";
        return true;
    }
    std::string processError;
    if (state.windowProcess.isRunning() &&
        !state.windowProcess.terminate(1000u, processError)) {
        errorBuffer.write(processError.empty() ? "failed to close previous fallback window"
                                               : processError);
        return false;
    }
    const std::string text =
        std::string("BLITZAR GUI fallback\n\n") +
        "Qt is not available in this build, so this lightweight window confirms that the GUI entry "
        "point is running.\n\nServer endpoint: " +
        state.host + ":" + std::to_string(state.port) +
        "\nConfig: " + state.configPath +
        "\n\nInstall/build Qt to enable the full in-process visual interface.";
    if (std::filesystem::exists("/usr/bin/zenity") &&
        state.windowProcess.launch("zenity",
                                   {"--info", "--title", "BLITZAR", "--width", "520",
                                    "--height", "260", "--text", text, "--ok-label", "Close"},
                                   true, processError)) {
        return true;
    }
    processError.clear();
    if (std::filesystem::exists("/usr/bin/xmessage") &&
        state.windowProcess.launch("xmessage", {"-center", "-title", "BLITZAR", text}, true,
                                   processError)) {
        return true;
    }
    errorBuffer.write(processError.empty() ? "no fallback window launcher found" : processError);
    return false;
}

bool launchProcess(GuiProxyState& state, const std::string& clientExecutable,
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

std::string runningCommandLine(const GuiProxyState& state)
{
    return state.clientProcess.commandLine();
}

std::string runningPidLabel(const GuiProxyState& state)
{
    if (!state.clientProcess.isRunning()) {
        return {};
    }
    return state.clientProcess.pidString();
}

bool parseUnsignedPort(std::string_view raw, std::uint16_t& outPort)
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

void printHelp()
{
    std::cout << "[module-gui-proxy] commands:\n"
              << "  help\n"
              << "  status\n"
              << "  set-endpoint <host> <port>\n"
              << "  set-autostart <true|false>\n"
              << "  show\n"
              << "  launch <client_executable_path>\n"
              << "  stop\n";
}

bool parseBool(std::string_view raw, bool& out)
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
