/*
 * @file runtime/client/runtime/CliBridge.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public client facade and transport argument handling.
 */

#include "client/runtime/CliBridge.hpp"
#include "FndConstants.hpp"
#include "CliRemoteSession.hpp"
#include "config/text/CfgParse.hpp"
#include "PltPaths.hpp"
#include "protocol/codec/PtcJsonCodec.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace bltzr_client {
const std::uint32_t kRemoteTimeoutMinMs = kRuntimeRemoteTimeoutMinMs;
const std::uint32_t kRemoteTimeoutMaxMs = kRuntimeRemoteTimeoutMaxMs;
const std::uint32_t kRemoteCommandTimeoutMsDefault = kRuntimeRemoteCommandTimeoutDefaultMs;
const std::uint32_t kRemoteStatusTimeoutMsDefault = kRuntimeRemoteStatusTimeoutDefaultMs;
const std::uint32_t kRemoteSnapshotTimeoutMsDefault = kRuntimeRemoteSnapshotTimeoutDefaultMs;

static std::string deriveDefaultServerExecutable(const std::vector<std::string_view>& rawArgs)
{
    const std::string defaultName(bltzr_platform::serverDefaultExecutableName());
    if (rawArgs.empty() || rawArgs[0].empty()) {
        return defaultName;
    }
    std::error_code ec;
    const std::filesystem::path directory = std::filesystem::path(rawArgs[0]).parent_path();
    if (!directory.empty()) {
        const std::filesystem::path candidate = directory / defaultName;
        if (std::filesystem::exists(candidate, ec) && !ec) {
            return candidate.string();
        }
    }
    return defaultName;
}

std::uint32_t clampRemoteTimeoutMs(std::uint32_t timeoutMs)
{
    return std::clamp(timeoutMs, kRemoteTimeoutMinMs, kRemoteTimeoutMaxMs);
}

bool parsePortValue(std::string_view raw, std::uint16_t& outPort)
{
    unsigned int parsed = 0u;
    if (!bltzr_text::parseNumber(raw, parsed) || parsed < kNetworkPortMin ||
        parsed > kNetworkPortMax) {
        return false;
    }
    outPort = static_cast<std::uint16_t>(parsed);
    return true;
}

bool parseBoolArg(std::string_view raw, bool& out)
{
    std::string normalized(raw);
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

bool splitTransportArgs(const std::vector<std::string_view>& rawArgs,
                        std::vector<std::string_view>& filteredArgs, TransportArgs& transport,
                        std::ostream& warnings)
{
    if (transport.serverExecutable.empty()) {
        transport.serverExecutable = deriveDefaultServerExecutable(rawArgs);
    }
    filteredArgs.clear();
    filteredArgs.reserve(rawArgs.size());
    if (!rawArgs.empty()) {
        filteredArgs.push_back(rawArgs[0]);
    }
    for (std::size_t i = 1u; i < rawArgs.size(); ++i) {
        const std::string raw(rawArgs[i]);
        if (raw == "--remote") {
            warnings
                << "[args] --remote is deprecated; client mode always uses the server service\n";
            continue;
        }
        if (raw == "--server-host") {
            if (i + 1u >= rawArgs.size()) {
                warnings << "[args] missing value for --server-host\n";
                return false;
            }
            transport.remoteHost = std::string(rawArgs[++i]);
            continue;
        }
        if (raw.rfind("--server-host=", 0u) == 0u) {
            transport.remoteHost = raw.substr(std::string("--server-host=").size());
            continue;
        }
        if (raw == "--server-port") {
            if (i + 1u >= rawArgs.size()) {
                warnings << "[args] missing value for --server-port\n";
                return false;
            }
            std::uint16_t parsedPort = transport.remotePort;
            if (!parsePortValue(rawArgs[++i], parsedPort)) {
                warnings << "[args] invalid --server-port value\n";
                return false;
            }
            transport.remotePort = parsedPort;
            continue;
        }
        if (raw.rfind("--server-port=", 0u) == 0u) {
            std::uint16_t parsedPort = transport.remotePort;
            const std::string value = raw.substr(std::string("--server-port=").size());
            if (!parsePortValue(value, parsedPort)) {
                warnings << "[args] invalid --server-port value: " << value << "\n";
                return false;
            }
            transport.remotePort = parsedPort;
            continue;
        }
        if (raw == "--server-autostart") {
            if (i + 1u < rawArgs.size()) {
                bool parsed = transport.remoteAutoStart;
                if (parseBoolArg(rawArgs[i + 1u], parsed)) {
                    transport.remoteAutoStart = parsed;
                    ++i;
                }
            }
            continue;
        }
        if (raw.rfind("--server-autostart=", 0u) == 0u) {
            bool parsed = transport.remoteAutoStart;
            const std::string value = raw.substr(std::string("--server-autostart=").size());
            if (!parseBoolArg(value, parsed)) {
                warnings << "[args] invalid --server-autostart value: " << value << "\n";
                return false;
            }
            transport.remoteAutoStart = parsed;
            continue;
        }
        if (raw == "--server-bin") {
            if (i + 1u >= rawArgs.size()) {
                warnings << "[args] missing value for --server-bin\n";
                return false;
            }
            transport.serverExecutable = std::string(rawArgs[++i]);
            continue;
        }
        if (raw.rfind("--server-bin=", 0u) == 0u) {
            transport.serverExecutable = raw.substr(std::string("--server-bin=").size());
            continue;
        }
        if (raw == "--server-token") {
            if (i + 1u >= rawArgs.size()) {
                warnings << "[args] missing value for --server-token\n";
                return false;
            }
            transport.remoteAuthToken = std::string(rawArgs[++i]);
            continue;
        }
        if (raw.rfind("--server-token=", 0u) == 0u) {
            transport.remoteAuthToken = raw.substr(std::string("--server-token=").size());
            continue;
        }
        filteredArgs.push_back(rawArgs[i]);
    }
    return true;
}

Bridge::Bridge(const std::string& configPath, std::string remoteHost, std::uint16_t remotePort,
               bool remoteAutoStart, std::string serverExecutable, std::string remoteAuthToken,
               std::uint32_t remoteCommandTimeoutMs, std::uint32_t remoteStatusTimeoutMs,
               std::uint32_t remoteSnapshotTimeoutMs)
    : _remote(std::make_unique<RemoteSession>(configPath, std::move(remoteHost), remotePort,
                                              remoteAutoStart, std::move(serverExecutable),
                                              std::move(remoteAuthToken), remoteCommandTimeoutMs,
                                              remoteStatusTimeoutMs, remoteSnapshotTimeoutMs)),
      _defaultExportFormat()
{
}

Bridge::~Bridge() = default;

bool Bridge::start()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _remote->start();
}

void Bridge::stop()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _remote->stop();
}

void Bridge::configureRemoteConnector(const std::string& host, std::uint16_t port, bool autoStart,
                                      const std::string& serverExecutable)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    (void)_remote->configure(host, port, autoStart, serverExecutable);
}

bool Bridge::tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot,
                                std::size_t* outSourceSize)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _remote->tryConsumeSnapshot(outSnapshot, outSourceSize);
}

SimulationStats Bridge::getStats()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _remote->getStats();
}

void Bridge::setRemoteSnapshotCap(std::uint32_t maxPoints)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _remote->setSnapshotCap(maxPoints);
}

void Bridge::requestReconnect()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _remote->requestReconnect();
}

bool Bridge::isRemoteMode() const
{
    return true;
}

bool Bridge::launchedByClient() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _remote->launchedByClient();
}

LinkState Bridge::linkState() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _remote->linkState();
}

std::string_view Bridge::linkStateLabel() const
{
    static thread_local std::string label;
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    label = _remote->linkStateLabel();
    return label;
}

std::string_view Bridge::serverOwnerLabel() const
{
    static thread_local std::string label;
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    label = _remote->serverOwnerLabel();
    return label;
}

std::string Bridge::jsonEscape(const std::string& value)
{
    return bltzr_protocol::JsonCodec::escapeString(value);
}

bool Bridge::sendOrQueueRemote(const std::string& command, const std::string& fields)
{
    return _remote->sendOrQueue(command, fields);
}
} // namespace bltzr_client
