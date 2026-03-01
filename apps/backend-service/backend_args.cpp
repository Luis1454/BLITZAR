#include "apps/backend-service/backend_args.hpp"

#include "config/SimulationArgs.hpp"
#include "config/TextParse.hpp"

#include <algorithm>
#include <atomic>
#include <csignal>
#include <cctype>
#include <iostream>
#include <string>

namespace grav_backend_service {

std::atomic<bool> g_stopRequested{false};

void onSignal(int)
{
    g_stopRequested.store(true);
}

bool parsePort(std::string_view token, std::uint16_t &out)
{
    unsigned int parsed = 0;
    if (!grav_text::parseNumber(token, parsed) || parsed == 0 || parsed > 65535u) {
        return false;
    }
    out = static_cast<std::uint16_t>(parsed);
    return true;
}

bool parseBool(std::string_view value, bool &out)
{
    std::string normalized(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (normalized == "1" || normalized == "true" || normalized == "on" || normalized == "yes") {
        out = true;
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "off" || normalized == "no") {
        out = false;
        return true;
    }
    return false;
}
bool parseBackendArgs(
    const std::vector<std::string_view> &rawArgs,
    DaemonOptions &outOptions,
    std::ostream &outError)
{
    outOptions = DaemonOptions{};
    outOptions.simArgs.reserve(rawArgs.size());
    if (!rawArgs.empty()) {
        outOptions.simArgs.push_back(rawArgs[0]);
    }

    for (std::size_t i = 1; i < rawArgs.size(); ++i) {
        const std::string token(rawArgs[i]);
        if (token == "--backend-help") {
            outOptions.showBackendHelp = true;
            continue;
        }
        if (token == "--backend-paused") {
            outOptions.startPaused = true;
            continue;
        }
        if (token == "--backend-allow-remote") {
            outOptions.allowRemoteBind = true;
            continue;
        }
        if (token.rfind("--backend-allow-remote=", 0) == 0) {
            bool parsed = outOptions.allowRemoteBind;
            const std::string rawValue = token.substr(std::string("--backend-allow-remote=").size());
            if (!parseBool(rawValue, parsed)) {
                outError << "[backend] invalid --backend-allow-remote value\n";
                return false;
            }
            outOptions.allowRemoteBind = parsed;
            continue;
        }
        if (token == "--backend-token") {
            if (i + 1 >= rawArgs.size()) {
                outError << "[backend] missing value after --backend-token\n";
                return false;
            }
            outOptions.authToken = std::string(rawArgs[++i]);
            continue;
        }
        if (token.rfind("--backend-token=", 0) == 0) {
            outOptions.authToken = token.substr(std::string("--backend-token=").size());
            continue;
        }
        if (token.rfind("--backend-host=", 0) == 0) {
            outOptions.host = token.substr(std::string("--backend-host=").size());
            continue;
        }
        if (token == "--backend-host") {
            if (i + 1 >= rawArgs.size()) {
                outError << "[backend] missing value after --backend-host\n";
                return false;
            }
            outOptions.host = std::string(rawArgs[++i]);
            continue;
        }
        if (token.rfind("--backend-port=", 0) == 0) {
            std::uint16_t parsed = 0;
            if (!parsePort(std::string_view(token).substr(std::string("--backend-port=").size()), parsed)) {
                outError << "[backend] invalid --backend-port value\n";
                return false;
            }
            outOptions.port = parsed;
            continue;
        }
        if (token == "--backend-port") {
            if (i + 1 >= rawArgs.size()) {
                outError << "[backend] missing value after --backend-port\n";
                return false;
            }
            std::uint16_t parsed = 0;
            if (!parsePort(rawArgs[++i], parsed)) {
                outError << "[backend] invalid --backend-port value\n";
                return false;
            }
            outOptions.port = parsed;
            continue;
        }
        outOptions.simArgs.push_back(rawArgs[i]);
    }

    return true;
}

void printBackendHelp(std::string_view programName)
{
    std::cout
        << "Backend daemon options:\n"
        << "  --backend-host <ipv4>     Bind address (default: 127.0.0.1)\n"
        << "  --backend-port <1..65535> TCP port (default: 4545)\n"
        << "  --backend-token <secret>  Optional auth token required on every request\n"
        << "  --backend-allow-remote    Allow non-loopback bind host (explicit opt-in)\n"
        << "  --backend-paused          Start simulation paused\n"
        << "  --backend-help            Show this backend-specific help\n"
        << "\n";
    printUsage(std::cout, programName, true);
}

bool isLoopbackBindHost(std::string_view host)
{
    if (host.empty() || host == "localhost") {
        return true;
    }
    return host.rfind("127.", 0) == 0;
}

void installStopSignalHandlers()
{
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);
}

bool stopRequested()
{
    return g_stopRequested.load();
}

void resetStopRequested()
{
    g_stopRequested.store(false);
}

} // namespace grav_backend_service
