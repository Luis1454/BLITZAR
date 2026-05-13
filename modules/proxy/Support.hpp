/*
 * @file modules/proxy/Support.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Internal support helpers for the GUI proxy client module.
 */

#ifndef BLITZAR_MODULES_PROXY_SUPPORT_HPP_
#define BLITZAR_MODULES_PROXY_SUPPORT_HPP_
#include "Constants.hpp"
#include "client/diagnostics/ErrorBuffer.hpp"
#include "platform/Process.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct GuiProxyState {
    std::string configPath = "simulation.ini";
    std::string host = kDefaultLoopbackHost;
    std::uint16_t port = kDefaultServerPort;
    bool autoStartServer = false;
    bltzr_platform::ProcessHandle clientProcess;
    bltzr_platform::ProcessHandle windowProcess;
};

std::string trim(const std::string& input);
std::vector<std::string> splitTokens(const std::string& line);
std::string proxyError(std::string_view operation, std::string_view detail);
void writeProxyError(const bltzr_client::ErrorBufferView& errorBuffer, std::string_view operation,
                     std::string_view detail);
bool isRunning(const GuiProxyState& state);
bool isWindowRunning(const GuiProxyState& state);
bool stopProcess(GuiProxyState& state, const bltzr_client::ErrorBufferView& errorBuffer);
bool launchProcess(GuiProxyState& state, const std::string& clientExecutable,
                   const bltzr_client::ErrorBufferView& errorBuffer);
bool launchFallbackWindow(GuiProxyState& state, const bltzr_client::ErrorBufferView& errorBuffer);
std::string runningCommandLine(const GuiProxyState& state);
std::string runningPidLabel(const GuiProxyState& state);
bool parseUnsignedPort(std::string_view raw, std::uint16_t& outPort);
void printHelp();
bool parseBool(std::string_view raw, bool& out);

#endif // BLITZAR_MODULES_PROXY_SUPPORT_HPP_
