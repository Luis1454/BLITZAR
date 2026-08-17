/*
 * @file runtime/client/runtime/CliRemoteSession.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Remote transport, retry, and snapshot session implementation.
 */

#include "CliRemoteSession.hpp"
#include "FndConstants.hpp"
#include "PltPaths.hpp"
#include "PltProcess.hpp"
#include "protocol/PtcProtocol.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace bltzr_client {
static constexpr auto kReconnectRetryIntervalMin = std::chrono::milliseconds(50);
static constexpr auto kReconnectRetryIntervalMax = std::chrono::milliseconds(1000);
static constexpr auto kErrorLogInterval = std::chrono::milliseconds(1500);
static const std::string_view kServerDefaultName = bltzr_platform::serverDefaultExecutableName();

class SocketTimeoutScope final {
public:
    SocketTimeoutScope(bltzr_protocol::Client& client, int timeoutMs)
        : _client(client), _previousTimeoutMs(client.socketTimeoutMs())
    {
        _client.setSocketTimeoutMs(timeoutMs);
    }

    ~SocketTimeoutScope()
    {
        _client.setSocketTimeoutMs(_previousTimeoutMs);
    }

private:
    bltzr_protocol::Client& _client;
    int _previousTimeoutMs;
};

static bool isTransportFailure(std::string_view reason)
{
    return reason == "not connected" || reason == "send failed" || reason == "recv failed" ||
           reason == "invalid response";
}

static bool isLoopbackHost(std::string_view host)
{
    return host.empty() || host == kDefaultLoopbackHost || host == "localhost";
}

static std::string defaultServerExecutable(const std::string& host, const std::string& executable)
{
    if (!executable.empty()) {
        return executable;
    }
    static_cast<void>(host);
    return std::string(kServerDefaultName);
}

RemoteSession::RemoteSession(const std::string& configPath, std::string host, std::uint16_t port,
                             bool autoStart, std::string serverExecutable, std::string authToken,
                             std::uint32_t commandTimeoutMs, std::uint32_t statusTimeoutMs,
                             std::uint32_t snapshotTimeoutMs)
    : _configPath(configPath),
      _host(std::move(host)),
      _port(port),
      _autoStart(autoStart),
      _serverExecutable(defaultServerExecutable(_host, serverExecutable)),
      _authToken(std::move(authToken)),
      _client(),
      _launchAttempted(false),
      _state(),
      _cachedStats{},
      _lastReconnectAttempt(std::chrono::steady_clock::time_point::min()),
      _lastReconnectErrorLog(std::chrono::steady_clock::time_point::min()),
      _lastRemoteErrorLog(std::chrono::steady_clock::time_point::min()),
      _retryDelay(kReconnectRetryIntervalMin),
      _commandTimeoutMs(clampRemoteTimeoutMs(commandTimeoutMs)),
      _statusTimeoutMs(clampRemoteTimeoutMs(statusTimeoutMs)),
      _snapshotTimeoutMs(clampRemoteTimeoutMs(snapshotTimeoutMs))
{
    _client.setSocketTimeoutMs(static_cast<int>(_commandTimeoutMs));
    _client.setAuthToken(_authToken);
}

bool RemoteSession::start()
{
    if (_state.isConnected() && _client.isConnected()) {
        return true;
    }
    const bool autoStartCapable = shouldAutoStart();
    if (!ensureConnected(true)) {
        return autoStartCapable;
    }
    refreshStats();
    return _state.isConnected() && _client.isConnected();
}

void RemoteSession::stop()
{
    if (_state.serverLaunched() && _client.isConnected()) {
        (void)_client.sendCommand(std::string(bltzr_protocol::Shutdown));
    }
    if (_state.isConnected()) {
        _client.disconnect();
    }
    _state.setConnected(false);
    _state.setServerLaunched(false);
}

bool RemoteSession::sendOrQueue(const std::string& command, const std::string& fields)
{
    if (!_state.isConnected() || !_client.isConnected()) {
        queueCommand(command, fields);
        return true;
    }
    if (sendNow(command, fields)) {
        return true;
    }
    queueCommand(command, fields);
    return true;
}

bool RemoteSession::configure(const std::string& host, std::uint16_t port, bool autoStart,
                              const std::string& serverExecutable)
{
    if (!host.empty()) {
        _host = host;
    }
    _port = (port == 0u) ? kDefaultServerPort : port;
    _autoStart = autoStart;
    if (!serverExecutable.empty()) {
        _serverExecutable = serverExecutable;
    }
    _client.setAuthToken(_authToken);
    _client.disconnect();
    _state.setConnected(false);
    _launchAttempted = false;
    _state.setServerLaunched(false);
    _state.clearPendingCommands();
    _retryDelay = kReconnectRetryIntervalMin;
    _lastReconnectAttempt = std::chrono::steady_clock::time_point::min();
    _lastReconnectErrorLog = std::chrono::steady_clock::time_point::min();
    _lastRemoteErrorLog = std::chrono::steady_clock::time_point::min();
    return ensureConnected(true);
}

bool RemoteSession::tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot,
                                       std::size_t* outSourceSize)
{
    if (!ensureConnected(false)) {
        return false;
    }
    std::vector<RenderParticle> remoteSnapshot;
    std::size_t sourceSize = 0u;
    SocketTimeoutScope timeoutScope(_client, static_cast<int>(_snapshotTimeoutMs));
    const bltzr_protocol::Response response =
        _client.getSnapshot(remoteSnapshot, _state.remoteSnapshotCap(), &sourceSize);
    if (!response.ok) {
        if (isTransportFailure(response.error)) {
            markDisconnected("get_snapshot", response.error);
        }
        else {
            const auto now = std::chrono::steady_clock::now();
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
    if (outSourceSize != nullptr) {
        *outSourceSize = sourceSize;
    }
    outSnapshot = std::move(remoteSnapshot);
    return true;
}

SimulationStats RemoteSession::getStats()
{
    refreshStats();
    return _cachedStats;
}

void RemoteSession::setSnapshotCap(std::uint32_t maxPoints)
{
    const std::uint32_t clamped = _state.setRemoteSnapshotCap(maxPoints);
    (void)sendOrQueue(std::string(bltzr_protocol::SetSnapshotTransferCap),
                      "\"max_points\":" + std::to_string(clamped));
}

void RemoteSession::requestReconnect()
{
    _client.disconnect();
    _state.setConnected(false);
    if (_autoStart) {
        _launchAttempted = false;
    }
    _state.setServerLaunched(false);
    _state.clearPendingCommands();
    _retryDelay = kReconnectRetryIntervalMin;
    _lastReconnectAttempt = std::chrono::steady_clock::time_point::min();
    (void)ensureConnected(true);
}

bool RemoteSession::isConnected() const
{
    return _state.isConnected() && _client.isConnected();
}

bool RemoteSession::launchedByClient() const
{
    return _state.serverLaunched();
}

LinkState RemoteSession::linkState() const
{
    return isConnected() ? LinkState::Connected : LinkState::Reconnecting;
}

std::string RemoteSession::linkStateLabel() const
{
    return isConnected() ? "connected" : "reconnecting";
}

std::string RemoteSession::serverOwnerLabel() const
{
    return _state.serverOwnerLabel();
}

bool RemoteSession::shouldAutoStart() const
{
    return _autoStart && !_launchAttempted && isLoopbackHost(_host);
}

SimulationStats RemoteSession::fromRemoteStatus(const bltzr_protocol::ClientStatus& status)
{
    SimulationStats stats{};
    stats.steps = status.steps;
    stats.dt = status.dt;
    stats.totalTime = status.totalTime;
    stats.paused = status.paused;
    stats.faulted = status.faulted;
    stats.faultStep = status.faultStep;
    stats.faultReason = status.faultReason;
    stats.sphEnabled = status.sphEnabled;
    stats.serverFps = status.serverFps;
    stats.performanceProfile = status.performanceProfile;
    stats.substepTargetDt = status.substepTargetDt;
    stats.substepDt = status.substepDt;
    stats.substeps = status.substeps;
    stats.maxSubsteps = status.maxSubsteps;
    stats.snapshotPublishPeriodMs = status.snapshotPublishPeriodMs;
    stats.particleCount = status.particleCount;
    stats.totalMass = status.totalMass;
    stats.kineticEnergy = status.kineticEnergy;
    stats.potentialEnergy = status.potentialEnergy;
    stats.thermalEnergy = status.thermalEnergy;
    stats.radiatedEnergy = status.radiatedEnergy;
    stats.totalEnergy = status.totalEnergy;
    stats.energyDriftPct = status.energyDriftPct;
    stats.energyEstimated = status.energyEstimated;
    stats.solverName = status.solver;
    stats.integratorName = status.integrator;
    stats.gpuTelemetryEnabled = status.gpuTelemetryEnabled;
    stats.gpuTelemetryAvailable = status.gpuTelemetryAvailable;
    stats.gpuKernelMs = status.gpuKernelMs;
    stats.gpuCopyMs = status.gpuCopyMs;
    stats.gpuVramUsedBytes = status.gpuVramUsedBytes;
    stats.gpuVramTotalBytes = status.gpuVramTotalBytes;
    stats.exportQueueDepth = status.exportQueueDepth;
    stats.exportActive = status.exportActive;
    stats.exportCompletedCount = status.exportCompletedCount;
    stats.exportFailedCount = status.exportFailedCount;
    stats.exportLastState = status.exportLastState;
    stats.exportLastPath = status.exportLastPath;
    stats.exportLastMessage = status.exportLastMessage;
    return stats;
}

bool RemoteSession::sendNow(const std::string& command, const std::string& fields)
{
    if (!ensureConnected(false)) {
        return false;
    }
    SocketTimeoutScope timeoutScope(_client, static_cast<int>(_commandTimeoutMs));
    const bltzr_protocol::Response response = _client.sendCommand(command, fields);
    if (!response.ok) {
        if (isTransportFailure(response.error)) {
            markDisconnected(command, response.error);
            return false;
        }
        const auto now = std::chrono::steady_clock::now();
        if ((now - _lastRemoteErrorLog) >= kErrorLogInterval) {
            std::cerr << "[client] remote " << command << " rejected: " << response.error << "\n";
            _lastRemoteErrorLog = now;
        }
    }
    return true;
}

bool RemoteSession::ensureConnected(bool forceLog)
{
    if (_state.isConnected() && !_client.isConnected()) {
        _state.setConnected(false);
    }
    if (_state.isConnected()) {
        return true;
    }
    const auto now = std::chrono::steady_clock::now();
    if (!forceLog && (now - _lastReconnectAttempt) < _retryDelay) {
        return false;
    }
    _lastReconnectAttempt = now;
    if (_client.connect(_host, _port)) {
        _state.setConnected(true);
        _launchAttempted = true;
        _retryDelay = kReconnectRetryIntervalMin;
        flushCommands();
        return true;
    }
    if (_retryDelay < kReconnectRetryIntervalMax) {
        _retryDelay = std::min(_retryDelay * 2, kReconnectRetryIntervalMax);
    }
    autoStartServer();
    if (forceLog || (now - _lastReconnectErrorLog) >= kErrorLogInterval) {
        std::cerr << "[client] server unreachable " << _host << ":" << _port << " (retrying)\n";
        _lastReconnectErrorLog = now;
    }
    return false;
}

void RemoteSession::markDisconnected(const std::string& context, const std::string& reason)
{
    const auto now = std::chrono::steady_clock::now();
    const bool logThisError =
        (reason != "not connected") || ((now - _lastRemoteErrorLog) >= kErrorLogInterval);
    if (logThisError) {
        std::cerr << "[client] remote " << context << " failed: " << reason << " (reconnecting)\n";
        _lastRemoteErrorLog = now;
    }
    _client.disconnect();
    _state.setConnected(false);
    _retryDelay = kReconnectRetryIntervalMin;
    _lastReconnectAttempt = now;
}

void RemoteSession::autoStartServer()
{
    if (!shouldAutoStart()) {
        return;
    }
    _launchAttempted = true;
    const std::string executable = defaultServerExecutable(_host, _serverExecutable);
    std::vector<std::string> arguments = {"--config", _configPath,     "--server-host",
                                          _host,      "--server-port", std::to_string(_port)};
    if (!_authToken.empty()) {
        arguments.push_back("--server-token");
        arguments.push_back(_authToken);
    }
    std::string launchError;
    if (bltzr_platform::launchDetachedProcess(executable, arguments, launchError)) {
        _state.setServerLaunched(true);
        std::cout << "[client] auto-start server: " << executable << " (" << _host << ":" << _port
                  << ")\n";
        return;
    }
    std::cerr << "[client] server auto-start failed ("
              << (launchError.empty() ? "unknown error" : launchError)
              << "), run manually: " << executable << " --server-host " << _host
              << " --server-port " << _port << "\n";
}

void RemoteSession::queueCommand(const std::string& command, const std::string& fields)
{
    if (_state.queuePendingCommand(command, fields)) {
        std::cerr << "[client] remote queue full; dropping oldest queued command\n";
    }
}

void RemoteSession::flushCommands()
{
    std::size_t sentCount = 0u;
    const std::size_t pendingCount = _state.pendingCommandCount();
    for (; sentCount < pendingCount; ++sentCount) {
        const std::pair<std::string, std::string> command = _state.pendingCommandAt(sentCount);
        if (!sendNow(command.first, command.second)) {
            break;
        }
    }
    _state.erasePendingPrefix(sentCount);
}

void RemoteSession::refreshStats()
{
    if (!ensureConnected(false)) {
        return;
    }
    bltzr_protocol::ClientStatus status{};
    SocketTimeoutScope timeoutScope(_client, static_cast<int>(_statusTimeoutMs));
    const bltzr_protocol::Response response = _client.getStatus(status);
    if (!response.ok) {
        if (isTransportFailure(response.error)) {
            markDisconnected("status", response.error);
        }
        else {
            const auto now = std::chrono::steady_clock::now();
            if ((now - _lastRemoteErrorLog) >= kErrorLogInterval) {
                std::cerr << "[client] remote status rejected: " << response.error << "\n";
                _lastRemoteErrorLog = now;
            }
        }
        return;
    }
    _cachedStats = fromRemoteStatus(status);
}
} // namespace bltzr_client
