#include "client/ClientServerBridge.hpp"

#include "protocol/ServerJsonCodec.hpp"
#include "protocol/ServerProtocol.hpp"
#include "platform/PlatformPaths.hpp"
#include "platform/PlatformProcess.hpp"
#include "config/TextParse.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <iostream>

namespace grav_client {

const std::uint32_t kClientRemoteTimeoutMinMs = 10u;
const std::uint32_t kClientRemoteTimeoutMaxMs = 60000u;
const std::uint32_t kClientRemoteCommandTimeoutMsDefault = 80u;
const std::uint32_t kClientRemoteStatusTimeoutMsDefault = 40u;
const std::uint32_t kClientRemoteSnapshotTimeoutMsDefault = 140u;

typedef std::chrono::steady_clock Clock;
constexpr auto kReconnectRetryIntervalMin = std::chrono::milliseconds(50);
constexpr auto kReconnectRetryIntervalMax = std::chrono::milliseconds(1000);
constexpr auto kErrorLogInterval = std::chrono::milliseconds(1500);
constexpr std::size_t kPendingRemoteCommandsMax = 256u;

const std::string_view kServerDefaultName = grav_platform::serverDefaultExecutableName();

bool parsePortValue(std::string_view raw, std::uint16_t &outPort)
{
    unsigned int parsed = 0u;
    if (!grav_text::parseNumber(raw, parsed) || parsed == 0u || parsed > 65535u) {
        return false;
    }
    outPort = static_cast<std::uint16_t>(parsed);
    return true;
}

bool parseBoolArg(std::string_view raw, bool &out)
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

bool isTransportClientFailure(std::string_view reason)
{
    return reason == "not connected"
        || reason == "send failed"
        || reason == "recv failed"
        || reason == "invalid response";
}

std::string deriveDefaultServerExecutable(const std::vector<std::string_view> &rawArgs)
{
    if (rawArgs.empty() || rawArgs[0].empty()) {
        return std::string(kServerDefaultName);
    }
    std::error_code ec;
    const std::filesystem::path executablePath(rawArgs[0]);
    const std::filesystem::path dir = executablePath.parent_path();
    if (!dir.empty()) {
        const std::filesystem::path candidate = dir / std::string(kServerDefaultName);
        if (std::filesystem::exists(candidate, ec) && !ec) {
            return candidate.string();
        }
    }
    return std::string(kServerDefaultName);
}

class SocketTimeoutScope {
    public:
        SocketTimeoutScope(ServerClient &client, int timeoutMs)
            : _client(client),
              _previousTimeoutMs(client.socketTimeoutMs())
        {
            _client.setSocketTimeoutMs(timeoutMs);
        }

        ~SocketTimeoutScope()
        {
            _client.setSocketTimeoutMs(_previousTimeoutMs);
        }

    private:
        ServerClient &_client;
        int _previousTimeoutMs;
};

template <typename LocalCall, typename RemoteCall>
void dispatchServerCall(
    bool remoteMode,
    const std::unique_ptr<ILocalServer> &localServer,
    LocalCall &&localCall,
    RemoteCall &&remoteCall)
{
    if (!remoteMode) {
        if (localServer) {
            localCall(*localServer);
        }
        return;
    }
    remoteCall();
}
std::uint32_t clampClientRemoteTimeoutMs(std::uint32_t timeoutMs)
{
    return std::clamp(timeoutMs, kClientRemoteTimeoutMinMs, kClientRemoteTimeoutMaxMs);
}

bool splitClientTransportArgs(
    const std::vector<std::string_view> &rawArgs,
    std::vector<std::string_view> &filteredArgs,
    ClientTransportArgs &transport,
    std::ostream &warnings)
{
    if (transport.serverExecutable.empty()) {
        transport.serverExecutable = deriveDefaultServerExecutable(rawArgs);
    }
    filteredArgs.clear();
    filteredArgs.reserve(rawArgs.size());
    if (!rawArgs.empty()) {
        filteredArgs.push_back(rawArgs[0]);
    }

    for (std::size_t i = 1; i < rawArgs.size(); ++i) {
        const std::string raw(rawArgs[i]);
        if (raw == "--remote") {
            transport.remoteMode = true;
            continue;
        }
        if (raw == "--server-host") {
            if (i + 1 >= rawArgs.size()) {
                warnings << "[args] missing value for --server-host\n";
                return false;
            }
            transport.remoteHost = std::string(rawArgs[++i]);
            transport.remoteMode = true;
            continue;
        }
        if (raw.rfind("--server-host=", 0) == 0) {
            transport.remoteHost = raw.substr(std::string("--server-host=").size());
            transport.remoteMode = true;
            continue;
        }
        if (raw == "--server-port") {
            if (i + 1 >= rawArgs.size()) {
                warnings << "[args] missing value for --server-port\n";
                return false;
            }
            std::uint16_t parsedPort = transport.remotePort;
            if (!parsePortValue(rawArgs[++i], parsedPort)) {
                warnings << "[args] invalid --server-port value\n";
                return false;
            }
            transport.remotePort = parsedPort;
            transport.remoteMode = true;
            continue;
        }
        if (raw.rfind("--server-port=", 0) == 0) {
            std::uint16_t parsedPort = transport.remotePort;
            const std::string value = raw.substr(std::string("--server-port=").size());
            if (!parsePortValue(value, parsedPort)) {
                warnings << "[args] invalid --server-port value: " << value << "\n";
                return false;
            }
            transport.remotePort = parsedPort;
            transport.remoteMode = true;
            continue;
        }
        if (raw == "--server-autostart") {
            if (i + 1 < rawArgs.size()) {
                bool parsed = transport.remoteAutoStart;
                if (parseBoolArg(rawArgs[i + 1], parsed)) {
                    transport.remoteAutoStart = parsed;
                    ++i;
                }
            }
            continue;
        }
        if (raw.rfind("--server-autostart=", 0) == 0) {
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
            if (i + 1 >= rawArgs.size()) {
                warnings << "[args] missing value for --server-bin\n";
                return false;
            }
            transport.serverExecutable = std::string(rawArgs[++i]);
            continue;
        }
        if (raw.rfind("--server-bin=", 0) == 0) {
            transport.serverExecutable = raw.substr(std::string("--server-bin=").size());
            continue;
        }
        if (raw == "--server-token") {
            if (i + 1 >= rawArgs.size()) {
                warnings << "[args] missing value for --server-token\n";
                return false;
            }
            transport.remoteAuthToken = std::string(rawArgs[++i]);
            transport.remoteMode = true;
            continue;
        }
        if (raw.rfind("--server-token=", 0) == 0) {
            transport.remoteAuthToken = raw.substr(std::string("--server-token=").size());
            transport.remoteMode = true;
            continue;
        }

        filteredArgs.push_back(rawArgs[i]);
    }

    return true;
}

ClientServerBridge::ClientServerBridge(
    const std::string &configPath,
    bool remoteMode,
    std::string remoteHost,
    std::uint16_t remotePort,
    bool remoteAutoStart,
    std::string serverExecutable,
    std::string remoteAuthToken,
    std::uint32_t remoteCommandTimeoutMs,
    std::uint32_t remoteStatusTimeoutMs,
    std::uint32_t remoteSnapshotTimeoutMs,
    LocalServerFactory localServerFactory)
    : _configPath(configPath),
      _remoteMode(remoteMode),
      _localServerFactory(std::move(localServerFactory)),
      _remoteHost(std::move(remoteHost)),
      _remotePort(remotePort),
      _remoteAutoStart(remoteAutoStart),
      _serverExecutable(serverExecutable.empty() ? std::string(kServerDefaultName) : std::move(serverExecutable)),
      _remoteAuthToken(std::move(remoteAuthToken)),
      _localServer(nullptr),
      _remoteClient(),
      _remoteConnected(false),
      _remoteLaunchAttempted(false),
      _remoteServerLaunched(false),
      _pendingQueueDropWarned(false),
      _pendingRemoteCommands(),
      _remoteSnapshotCap(4096u),
      _cachedStats{},
      _warnedRemoteInitialConfig(false),
      _defaultExportFormat(),
      _lastReconnectAttempt(Clock::time_point::min()),
      _lastReconnectErrorLog(Clock::time_point::min()),
      _lastRemoteErrorLog(Clock::time_point::min()),
      _reconnectRetryDelay(kReconnectRetryIntervalMin),
      _remoteCommandTimeoutMs(clampClientRemoteTimeoutMs(remoteCommandTimeoutMs)),
      _remoteStatusTimeoutMs(clampClientRemoteTimeoutMs(remoteStatusTimeoutMs)),
      _remoteSnapshotTimeoutMs(clampClientRemoteTimeoutMs(remoteSnapshotTimeoutMs))
{
    if (!_remoteMode && _localServerFactory) {
        _localServer = _localServerFactory(configPath);
    }
    if (_remoteMode) {
        _remoteClient.setSocketTimeoutMs(static_cast<int>(_remoteCommandTimeoutMs));
        _remoteClient.setAuthToken(_remoteAuthToken);
    }
}

bool ClientServerBridge::start()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!_remoteMode) {
        if (_localServer == nullptr && _localServerFactory) {
            _localServer = _localServerFactory(_configPath);
        }
        if (_localServer != nullptr) {
            _localServer->start();
            return true;
        }
        return false;
    }
    if (_remoteConnected && _remoteClient.isConnected()) {
        return true;
    }
    if (!ensureRemoteConnected(true)) {
        return false;
    }
    refreshRemoteStats();
    return _remoteConnected && _remoteClient.isConnected();
}

void ClientServerBridge::stop()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_remoteMode) {
        if (_remoteServerLaunched && _remoteClient.isConnected()) {
            (void)_remoteClient.sendCommand(std::string(grav_protocol::Shutdown));
        }
        if (_remoteConnected) {
            _remoteClient.disconnect();
        }
        _remoteConnected = false;
        _remoteServerLaunched = false;
        return;
    }
    if (_localServer != nullptr) {
        _localServer->stop();
    }
}

void ClientServerBridge::setPaused(bool paused)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [paused](ILocalServer &server) { server.setPaused(paused); },
        [this, paused]() {
            sendOrQueueRemote(std::string(paused ? grav_protocol::Pause : grav_protocol::Resume));
        });
}

void ClientServerBridge::togglePaused()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [](ILocalServer &server) { server.togglePaused(); },
        [this]() { sendOrQueueRemote(std::string(grav_protocol::Toggle)); });
}

void ClientServerBridge::stepOnce()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [](ILocalServer &server) { server.stepOnce(); },
        [this]() { sendOrQueueRemote(std::string(grav_protocol::Step), "\"count\":1"); });
}

void ClientServerBridge::setParticleCount(std::uint32_t particleCount)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [particleCount](ILocalServer &server) { server.setParticleCount(particleCount); },
        [this, particleCount]() {
            const std::uint32_t clamped = std::max<std::uint32_t>(2u, particleCount);
            sendOrQueueRemote(std::string(grav_protocol::SetParticleCount), "\"value\":" + std::to_string(clamped));
        });
}

void ClientServerBridge::setDt(float dt)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [dt](ILocalServer &server) { server.setDt(dt); },
        [this, dt]() {
            const float clamped = std::max(1e-6f, dt);
            sendOrQueueRemote(std::string(grav_protocol::SetDt), "\"value\":" + std::to_string(clamped));
        });
}

void ClientServerBridge::scaleDt(float factor)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [factor](ILocalServer &server) { server.scaleDt(factor); },
        [this, factor]() {
            const float currentDt = std::max(1e-6f, getStats().dt);
            const float scaled = std::max(1e-6f, currentDt * factor);
            setDt(scaled);
        });
}

void ClientServerBridge::requestReset()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [](ILocalServer &server) { server.requestReset(); },
        [this]() { sendOrQueueRemote(std::string(grav_protocol::Reset)); });
}

void ClientServerBridge::requestRecover()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [](ILocalServer &server) { server.requestReset(); },
        [this]() { sendOrQueueRemote(std::string(grav_protocol::Recover)); });
}

void ClientServerBridge::setSolverMode(const std::string &mode)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [&mode](ILocalServer &server) { server.setSolverMode(mode); },
        [this, &mode]() {
            sendOrQueueRemote(std::string(grav_protocol::SetSolver), "\"value\":\"" + jsonEscape(mode) + "\"");
        });
}

void ClientServerBridge::setIntegratorMode(const std::string &mode)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [&mode](ILocalServer &server) { server.setIntegratorMode(mode); },
        [this, &mode]() {
            sendOrQueueRemote(std::string(grav_protocol::SetIntegrator), "\"value\":\"" + jsonEscape(mode) + "\"");
        });
}

void ClientServerBridge::setOctreeParameters(float theta, float softening)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [theta, softening](ILocalServer &server) { server.setOctreeParameters(theta, softening); },
        [this, theta, softening]() {
            const float safeTheta = std::max(0.0001f, theta);
            const float safeSoftening = std::max(0.000001f, softening);
            sendOrQueueRemote(
                std::string(grav_protocol::SetOctree),
                "\"theta\":" + std::to_string(safeTheta) + ",\"softening\":" + std::to_string(safeSoftening));
        });
}

void ClientServerBridge::setSphEnabled(bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [enabled](ILocalServer &server) { server.setSphEnabled(enabled); },
        [this, enabled]() {
            sendOrQueueRemote(std::string(grav_protocol::SetSph), std::string("\"value\":") + (enabled ? "true" : "false"));
        });
}

void ClientServerBridge::setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [smoothingLength, restDensity, gasConstant, viscosity](ILocalServer &server) {
            server.setSphParameters(smoothingLength, restDensity, gasConstant, viscosity);
        },
        [this, smoothingLength, restDensity, gasConstant, viscosity]() {
            const float safeH = std::max(0.000001f, smoothingLength);
            const float safeRestDensity = std::max(0.000001f, restDensity);
            const float safeGasConstant = std::max(0.000001f, gasConstant);
            const float safeViscosity = std::max(0.0f, viscosity);
            sendOrQueueRemote(
                std::string(grav_protocol::SetSphParams),
                "\"h\":" + std::to_string(safeH)
                    + ",\"rest_density\":" + std::to_string(safeRestDensity)
                    + ",\"gas_constant\":" + std::to_string(safeGasConstant)
                    + ",\"viscosity\":" + std::to_string(safeViscosity));
        });
}

void ClientServerBridge::setInitialStateConfig(const InitialStateConfig &config)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [&config](ILocalServer &server) { server.setInitialStateConfig(config); },
        [this]() {
            if (!_warnedRemoteInitialConfig) {
                std::cout << "[client] remote server: initial state config templates are server-owned "
                             "(set/load through server API)\n";
                _warnedRemoteInitialConfig = true;
            }
        });
}

void ClientServerBridge::setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [everySteps, sampleLimit](ILocalServer &server) { server.setEnergyMeasurementConfig(everySteps, sampleLimit); },
        [this, everySteps, sampleLimit]() {
            const std::uint32_t safeEvery = std::max<std::uint32_t>(1u, everySteps);
            const std::uint32_t safeSampleLimit = std::max<std::uint32_t>(2u, sampleLimit);
            sendOrQueueRemote(
                std::string(grav_protocol::SetEnergyMeasure),
                "\"every_steps\":" + std::to_string(safeEvery) + ",\"sample_limit\":" + std::to_string(safeSampleLimit));
        });
}

void ClientServerBridge::setExportDefaults(const std::string &directory, const std::string &format)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [&directory, &format](ILocalServer &server) { server.setExportDefaults(directory, format); },
        [this, &format]() { _defaultExportFormat = format; });
}

void ClientServerBridge::setInitialStateFile(const std::string &path, const std::string &format)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [&path, &format](ILocalServer &server) { server.setInitialStateFile(path, format); },
        [this, &path, &format]() {
            if (!path.empty()) {
                sendOrQueueRemote(
                    std::string(grav_protocol::Load),
                    "\"path\":\"" + jsonEscape(path) + "\",\"format\":\"" + jsonEscape(format.empty() ? "auto" : format) + "\"");
            }
        });
}

void ClientServerBridge::requestExportSnapshot(const std::string &outputPath, const std::string &format)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [&outputPath, &format](ILocalServer &server) { server.requestExportSnapshot(outputPath, format); },
        [this, &outputPath, &format]() {
            const std::string effectiveFormat = format.empty() ? _defaultExportFormat : format;
            std::string fields;
            if (!outputPath.empty()) {
                fields = "\"path\":\"" + jsonEscape(outputPath) + "\"";
            }
            if (!effectiveFormat.empty()) {
                if (!fields.empty()) {
                    fields += ",";
                }
                fields += "\"format\":\"" + jsonEscape(effectiveFormat) + "\"";
            }
            sendOrQueueRemote(std::string(grav_protocol::Export), fields);
        });
}

void ClientServerBridge::requestShutdown()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchServerCall(
        _remoteMode,
        _localServer,
        [](ILocalServer &server) { server.stop(); },
        [this]() { sendOrQueueRemote(std::string(grav_protocol::Shutdown)); });
}

void ClientServerBridge::configureRemoteConnector(
    const std::string &host,
    std::uint16_t port,
    bool autoStart,
    const std::string &serverExecutable)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    if (!_remoteMode) {
        if (_localServer) {
            _localServer->stop();
            _localServer.reset();
        }
        _remoteMode = true;
        _remoteClient.setSocketTimeoutMs(static_cast<int>(_remoteCommandTimeoutMs));
        _remoteClient.setAuthToken(_remoteAuthToken);
    }

    if (!host.empty()) {
        _remoteHost = host;
    }
    _remotePort = (port == 0u) ? 4545u : port;
    _remoteAutoStart = autoStart;
    if (!serverExecutable.empty()) {
        _serverExecutable = serverExecutable;
    }
    _remoteClient.setAuthToken(_remoteAuthToken);

    _remoteClient.disconnect();
    _remoteConnected = false;
    _remoteLaunchAttempted = false;
    _remoteServerLaunched = false;
    _pendingRemoteCommands.clear();
    _pendingQueueDropWarned = false;
    _reconnectRetryDelay = kReconnectRetryIntervalMin;
    _lastReconnectAttempt = Clock::time_point::min();
    _lastReconnectErrorLog = Clock::time_point::min();
    _lastRemoteErrorLog = Clock::time_point::min();

    ensureRemoteConnected(true);
}

bool ClientServerBridge::tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!_remoteMode) {
        return _localServer != nullptr && _localServer->tryConsumeSnapshot(outSnapshot);
    }
    if (!ensureRemoteConnected(false)) {
        return false;
    }
    std::vector<RenderParticle> remoteSnapshot;
    SocketTimeoutScope timeoutScope(_remoteClient, static_cast<int>(_remoteSnapshotTimeoutMs));
    const ServerClientResponse response = _remoteClient.getSnapshot(
        remoteSnapshot,
        grav_protocol::clampSnapshotPoints(_remoteSnapshotCap));
    if (!response.ok) {
        if (isTransportClientFailure(response.error)) {
            markRemoteDisconnected("get_snapshot", response.error);
        } else {
            const auto now = Clock::now();
            if ((now - _lastRemoteErrorLog) >= kErrorLogInterval) {
                std::cerr << "[client] remote get_snapshot rejected: " << response.error << "\n";
                _lastRemoteErrorLog = now;
            }
        }
        return false;
    }
    if (remoteSnapshot.empty()) {
        return false;
    }
    outSnapshot = std::move(remoteSnapshot);
    return true;
}

SimulationStats ClientServerBridge::getStats()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!_remoteMode) {
        return _localServer != nullptr ? _localServer->getStats() : SimulationStats{};
    }
    refreshRemoteStats();
    return _cachedStats;
}

void ClientServerBridge::setRemoteSnapshotCap(std::uint32_t maxPoints)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _remoteSnapshotCap = grav_protocol::clampSnapshotPoints(maxPoints);
}

void ClientServerBridge::requestReconnect()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!_remoteMode) {
        return;
    }
    _remoteClient.disconnect();
    _remoteConnected = false;
    if (_remoteAutoStart) {
        _remoteLaunchAttempted = false;
    }
    _remoteServerLaunched = false;
    _pendingRemoteCommands.clear();
    _pendingQueueDropWarned = false;
    _reconnectRetryDelay = kReconnectRetryIntervalMin;
    _lastReconnectAttempt = Clock::time_point::min();
    ensureRemoteConnected(true);
}

bool ClientServerBridge::isRemoteMode() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _remoteMode;
}

bool ClientServerBridge::launchedByClient() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _remoteMode && _remoteServerLaunched;
}

ClientLinkState ClientServerBridge::linkState() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!_remoteMode) {
        return ClientLinkState::LocalEmbedded;
    }
    if (_remoteConnected && _remoteClient.isConnected()) {
        return ClientLinkState::Connected;
    }
    return ClientLinkState::Reconnecting;
}

std::string_view ClientServerBridge::linkStateLabel() const
{
    const ClientLinkState state = linkState();
    switch (state) {
        case ClientLinkState::LocalEmbedded:
            return "local";
        case ClientLinkState::Connected:
            return "connected";
        case ClientLinkState::Reconnecting:
        default:
            return "reconnecting";
    }
}

std::string_view ClientServerBridge::serverOwnerLabel() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!_remoteMode) {
        return "embedded";
    }
    if (_remoteServerLaunched) {
        return "managed";
    }
    return "external";
}

std::string ClientServerBridge::jsonEscape(const std::string &value)
{
    return grav_protocol::ServerJsonCodec::escapeString(value);
}

SimulationStats ClientServerBridge::fromRemoteStatus(const ServerClientStatus &status)
{
    SimulationStats stats{};
    stats.steps = status.steps;
    stats.dt = status.dt;
    stats.paused = status.paused;
    stats.faulted = status.faulted;
    stats.faultStep = status.faultStep;
    stats.faultReason = status.faultReason;
    stats.sphEnabled = status.sphEnabled;
    stats.serverFps = status.serverFps;
    stats.particleCount = status.particleCount;
    stats.kineticEnergy = status.kineticEnergy;
    stats.potentialEnergy = status.potentialEnergy;
    stats.thermalEnergy = status.thermalEnergy;
    stats.radiatedEnergy = status.radiatedEnergy;
    stats.totalEnergy = status.totalEnergy;
    stats.energyDriftPct = status.energyDriftPct;
    stats.energyEstimated = status.energyEstimated;
    stats.solverName = status.solver;
    stats.integratorName = status.integrator;
    return stats;
}

bool ClientServerBridge::sendRemoteNow(const std::string &cmd, const std::string &fields)
{
    if (!ensureRemoteConnected(false)) {
        return false;
    }
    SocketTimeoutScope timeoutScope(_remoteClient, static_cast<int>(_remoteCommandTimeoutMs));
    const ServerClientResponse response = _remoteClient.sendCommand(cmd, fields);
    if (!response.ok) {
        if (isTransportClientFailure(response.error)) {
            markRemoteDisconnected(cmd, response.error);
            return false;
        }
        const auto now = Clock::now();
        if ((now - _lastRemoteErrorLog) >= kErrorLogInterval) {
            std::cerr << "[client] remote " << cmd << " rejected: " << response.error << "\n";
            _lastRemoteErrorLog = now;
        }
        return true;
    }
    return true;
}

bool ClientServerBridge::sendOrQueueRemote(const std::string &cmd, const std::string &fields)
{
    if (!_remoteMode) {
        return false;
    }
    if (!_remoteConnected || !_remoteClient.isConnected()) {
        queuePendingRemoteCommand(cmd, fields);
        return true;
    }
    if (sendRemoteNow(cmd, fields)) {
        return true;
    }
    queuePendingRemoteCommand(cmd, fields);
    return true;
}

void ClientServerBridge::queuePendingRemoteCommand(const std::string &cmd, const std::string &fields)
{
    if (!_pendingRemoteCommands.empty()) {
        auto &last = _pendingRemoteCommands.back();
        if (last.first == cmd) {
            last.second = fields;
            return;
        }
    }
    if (_pendingRemoteCommands.size() >= kPendingRemoteCommandsMax) {
        _pendingRemoteCommands.erase(_pendingRemoteCommands.begin());
        if (!_pendingQueueDropWarned) {
            std::cerr << "[client] remote queue full; dropping oldest queued command\n";
            _pendingQueueDropWarned = true;
        }
    }
    _pendingRemoteCommands.emplace_back(cmd, fields);
}

bool ClientServerBridge::ensureRemoteConnected(bool forceLog)
{
    if (!_remoteMode) {
        return true;
    }

    if (_remoteConnected && !_remoteClient.isConnected()) {
        _remoteConnected = false;
    }
    if (_remoteConnected) {
        return true;
    }

    const auto now = Clock::now();
    if (!forceLog && (now - _lastReconnectAttempt) < _reconnectRetryDelay) {
        return false;
    }
    _lastReconnectAttempt = now;

    if (_remoteClient.connect(_remoteHost, _remotePort)) {
        _remoteConnected = true;
        _remoteLaunchAttempted = true;
        _reconnectRetryDelay = kReconnectRetryIntervalMin;
        flushPendingRemoteCommands();
        return true;
    }

    if (_reconnectRetryDelay < kReconnectRetryIntervalMax) {
        const auto grown = _reconnectRetryDelay * 2;
        _reconnectRetryDelay = std::min(grown, kReconnectRetryIntervalMax);
    }

    tryAutoStartRemoteServer();

    if (forceLog || (now - _lastReconnectErrorLog) >= kErrorLogInterval) {
        std::cerr << "[client] server unreachable " << _remoteHost << ":" << _remotePort << " (retrying)\n";
        _lastReconnectErrorLog = now;
    }
    return false;
}

void ClientServerBridge::markRemoteDisconnected(const std::string &context, const std::string &reason)
{
    const auto now = Clock::now();
    const bool logThisError =
        (reason != "not connected")
        || ((now - _lastRemoteErrorLog) >= kErrorLogInterval);
    if (logThisError) {
        std::cerr << "[client] remote " << context << " failed: " << reason << " (reconnecting)\n";
        _lastRemoteErrorLog = now;
    }
    _remoteClient.disconnect();
    _remoteConnected = false;
    _reconnectRetryDelay = kReconnectRetryIntervalMin;
    _lastReconnectAttempt = now;
}

bool ClientServerBridge::isLoopbackHost(std::string_view host)
{
    if (host.empty()) {
        return true;
    }
    return host == "127.0.0.1" || host == "localhost";
}

bool ClientServerBridge::shouldAutoStartRemoteServer() const
{
    return _remoteMode
        && _remoteAutoStart
        && !_remoteLaunchAttempted
        && isLoopbackHost(_remoteHost);
}

void ClientServerBridge::tryAutoStartRemoteServer()
{
    if (!shouldAutoStartRemoteServer()) {
        return;
    }
    _remoteLaunchAttempted = true;
    const std::string exe = _serverExecutable.empty() ? std::string(kServerDefaultName) : _serverExecutable;
    const std::vector<std::string> args = {
        "--config",
        _configPath,
        "--server-host",
        _remoteHost,
        "--server-port",
        std::to_string(_remotePort)
    };
    std::vector<std::string> effectiveArgs = args;
    if (!_remoteAuthToken.empty()) {
        effectiveArgs.push_back("--server-token");
        effectiveArgs.push_back(_remoteAuthToken);
    }
    std::string launchError;
    if (grav_platform::launchDetachedProcess(exe, effectiveArgs, launchError)) {
        _remoteServerLaunched = true;
        std::cout << "[client] auto-start server: " << exe << " (" << _remoteHost << ":" << _remotePort << ")\n";
    } else {
        std::cerr << "[client] server auto-start failed ("
                  << (launchError.empty() ? "unknown error" : launchError)
                  << "), run manually: " << exe << " --server-host " << _remoteHost
                  << " --server-port " << _remotePort << "\n";
    }
}

void ClientServerBridge::flushPendingRemoteCommands()
{
    std::size_t sentCount = 0u;
    for (; sentCount < _pendingRemoteCommands.size(); ++sentCount) {
        const auto &command = _pendingRemoteCommands[sentCount];
        if (!sendRemoteNow(command.first, command.second)) {
            break;
        }
    }
    if (sentCount > 0u) {
        _pendingRemoteCommands.erase(
            _pendingRemoteCommands.begin(),
            _pendingRemoteCommands.begin() + static_cast<std::ptrdiff_t>(sentCount));
    }
    if (_pendingRemoteCommands.empty()) {
        _pendingQueueDropWarned = false;
    }
}

void ClientServerBridge::refreshRemoteStats()
{
    if (!ensureRemoteConnected(false)) {
        return;
    }
    ServerClientStatus status{};
    SocketTimeoutScope timeoutScope(_remoteClient, static_cast<int>(_remoteStatusTimeoutMs));
    const ServerClientResponse response = _remoteClient.getStatus(status);
    if (!response.ok) {
        if (isTransportClientFailure(response.error)) {
            markRemoteDisconnected("status", response.error);
        } else {
            const auto now = Clock::now();
            if ((now - _lastRemoteErrorLog) >= kErrorLogInterval) {
                std::cerr << "[client] remote status rejected: " << response.error << "\n";
                _lastRemoteErrorLog = now;
            }
        }
        return;
    }
    _cachedStats = fromRemoteStatus(status);
}

} // namespace grav_client
