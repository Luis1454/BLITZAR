/*
 * @file runtime/src/client/ClientServerBridge.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "client/ClientServerBridge.hpp"
#include "config/TextParse.hpp"
#include "platform/PlatformPaths.hpp"
#include "platform/PlatformProcess.hpp"
#include "protocol/ServerJsonCodec.hpp"
#include "protocol/ServerProtocol.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <iostream>

namespace bltzr_client {
const std::uint32_t kClientRemoteTimeoutMinMs = 10u;
const std::uint32_t kClientRemoteTimeoutMaxMs = 60000u;
const std::uint32_t kClientRemoteCommandTimeoutMsDefault = 80u;
const std::uint32_t kClientRemoteStatusTimeoutMsDefault = 40u;
const std::uint32_t kClientRemoteSnapshotTimeoutMsDefault = 140u;
typedef std::chrono::steady_clock Clock;
constexpr auto kReconnectRetryIntervalMin = std::chrono::milliseconds(50);
constexpr auto kReconnectRetryIntervalMax = std::chrono::milliseconds(1000);
constexpr auto kErrorLogInterval = std::chrono::milliseconds(1500);
const std::string_view kServerDefaultName = bltzr_platform::serverDefaultExecutableName();

bool parsePortValue(std::string_view raw, std::uint16_t& outPort)
{
    unsigned int parsed = 0u;
    if (!bltzr_text::parseNumber(raw, parsed) || parsed == 0u || parsed > 65535u) {
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

bool isTransportClientFailure(std::string_view reason)
{
    return reason == "not connected" || reason == "send failed" || reason == "recv failed" ||
           reason == "invalid response";
}

std::string deriveDefaultServerExecutable(const std::vector<std::string_view>& rawArgs)
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
    SocketTimeoutScope(ServerClient& client, int timeoutMs)
        : _client(client), _previousTimeoutMs(client.socketTimeoutMs())
    {
        _client.setSocketTimeoutMs(timeoutMs);
    }

    ~SocketTimeoutScope()
    {
        _client.setSocketTimeoutMs(_previousTimeoutMs);
    }

private:
    ServerClient& _client;
    int _previousTimeoutMs;
};

std::uint32_t clampClientRemoteTimeoutMs(std::uint32_t timeoutMs)
{
    return std::clamp(timeoutMs, kClientRemoteTimeoutMinMs, kClientRemoteTimeoutMaxMs);
}

bool splitClientTransportArgs(const std::vector<std::string_view>& rawArgs,
                              std::vector<std::string_view>& filteredArgs,
                              ClientTransportArgs& transport, std::ostream& warnings)
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
            warnings
                << "[args] --remote is deprecated; client mode always uses the server service\n";
            continue;
        }
        if (raw == "--server-host") {
            if (i + 1 >= rawArgs.size()) {
                warnings << "[args] missing value for --server-host\n";
                return false;
            }
            transport.remoteHost = std::string(rawArgs[++i]);
            continue;
        }
        if (raw.rfind("--server-host=", 0) == 0) {
            transport.remoteHost = raw.substr(std::string("--server-host=").size());
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
            continue;
        }
        if (raw.rfind("--server-token=", 0) == 0) {
            transport.remoteAuthToken = raw.substr(std::string("--server-token=").size());
            continue;
        }
        filteredArgs.push_back(rawArgs[i]);
    }
    return true;
}

ClientServerBridge::ClientServerBridge(const std::string& configPath, std::string remoteHost,
                                       std::uint16_t remotePort, bool remoteAutoStart,
                                       std::string serverExecutable, std::string remoteAuthToken,
                                       std::uint32_t remoteCommandTimeoutMs,
                                       std::uint32_t remoteStatusTimeoutMs,
                                       std::uint32_t remoteSnapshotTimeoutMs)
    : _configPath(configPath),
      _remoteHost(std::move(remoteHost)),
      _remotePort(remotePort),
      _remoteAutoStart(remoteAutoStart),
      _serverExecutable(serverExecutable.empty() ? std::string(kServerDefaultName)
                                                 : std::move(serverExecutable)),
      _remoteAuthToken(std::move(remoteAuthToken)),
      _remoteClient(),
      _remoteLaunchAttempted(false),
      _runtimeState(),
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
    _remoteClient.setSocketTimeoutMs(static_cast<int>(_remoteCommandTimeoutMs));
    _remoteClient.setAuthToken(_remoteAuthToken);
}

bool ClientServerBridge::start()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_runtimeState.isConnected() && _remoteClient.isConnected()) {
        return true;
    }
    if (!ensureRemoteConnected(true)) {
        return false;
    }
    refreshRemoteStats();
    return _runtimeState.isConnected() && _remoteClient.isConnected();
}

void ClientServerBridge::stop()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_runtimeState.serverLaunched() && _remoteClient.isConnected()) {
        (void)_remoteClient.sendCommand(std::string(bltzr_protocol::Shutdown));
    }
    if (_runtimeState.isConnected()) {
        _remoteClient.disconnect();
    }
    _runtimeState.setConnected(false);
    _runtimeState.setServerLaunched(false);
}

void ClientServerBridge::setPaused(bool paused)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(paused ? bltzr_protocol::Pause : bltzr_protocol::Resume));
}

void ClientServerBridge::togglePaused()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::Toggle));
}

void ClientServerBridge::stepOnce()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::Step), "\"count\":1");
}

void ClientServerBridge::setParticleCount(std::uint32_t particleCount)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const std::uint32_t clamped = std::max<std::uint32_t>(2u, particleCount);
    sendOrQueueRemote(std::string(bltzr_protocol::SetParticleCount),
                      "\"value\":" + std::to_string(clamped));
}

void ClientServerBridge::setDt(float dt)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const float clamped = std::max(1e-6f, dt);
    sendOrQueueRemote(std::string(bltzr_protocol::SetDt), "\"value\":" + std::to_string(clamped));
}

void ClientServerBridge::scaleDt(float factor)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const float currentDt = std::max(1e-6f, getStats().dt);
    const float scaled = std::max(1e-6f, currentDt * factor);
    setDt(scaled);
}

void ClientServerBridge::requestReset()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::Reset));
}

void ClientServerBridge::requestRecover()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::Recover));
}

void ClientServerBridge::setSolverMode(const std::string& mode)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetSolver),
                      "\"value\":\"" + jsonEscape(mode) + "\"");
}

void ClientServerBridge::setIntegratorMode(const std::string& mode)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetIntegrator),
                      "\"value\":\"" + jsonEscape(mode) + "\"");
}

void ClientServerBridge::setPerformanceProfile(const std::string& profile)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetPerformanceProfile),
                      "\"value\":\"" + jsonEscape(profile) + "\"");
}

void ClientServerBridge::setOctreeParameters(float theta, float softening)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const float safeTheta = std::max(0.0001f, theta);
    const float safeSoftening = std::max(0.000001f, softening);
    sendOrQueueRemote(std::string(bltzr_protocol::SetOctree),
                      "\"theta\":" + std::to_string(safeTheta) +
                          ",\"softening\":" + std::to_string(safeSoftening));
}

void ClientServerBridge::setSphEnabled(bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetSph),
                      std::string("\"value\":") + (enabled ? "true" : "false"));
}

void ClientServerBridge::setSphParameters(float smoothingLength, float restDensity,
                                          float gasConstant, float viscosity)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const float safeH = std::max(0.000001f, smoothingLength);
    const float safeRestDensity = std::max(0.000001f, restDensity);
    const float safeGasConstant = std::max(0.000001f, gasConstant);
    const float safeViscosity = std::max(0.0f, viscosity);
    sendOrQueueRemote(std::string(bltzr_protocol::SetSphParams),
                      "\"h\":" + std::to_string(safeH) +
                          ",\"rest_density\":" + std::to_string(safeRestDensity) +
                          ",\"gas_constant\":" + std::to_string(safeGasConstant) +
                          ",\"viscosity\":" + std::to_string(safeViscosity));
}

void ClientServerBridge::setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const float safeTargetDt = std::max(0.0f, targetDt);
    const std::uint32_t safeMaxSubsteps = std::max<std::uint32_t>(1u, maxSubsteps);
    sendOrQueueRemote(std::string(bltzr_protocol::SetSubsteps),
                      "\"target_dt\":" + std::to_string(safeTargetDt) +
                          ",\"max_substeps\":" + std::to_string(safeMaxSubsteps));
}

void ClientServerBridge::setSnapshotPublishPeriodMs(std::uint32_t periodMs)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const std::uint32_t safePeriodMs = std::max<std::uint32_t>(1u, periodMs);
    sendOrQueueRemote(std::string(bltzr_protocol::SetSnapshotPublishCadence),
                      "\"period_ms\":" + std::to_string(safePeriodMs));
}

void ClientServerBridge::setInitialStateConfig(const InitialStateConfig& config)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    static_cast<void>(config);
    if (!_warnedRemoteInitialConfig) {
        std::cout << "[client] server-owned initial state config templates remain controlled by "
                     "the server API\n";
        _warnedRemoteInitialConfig = true;
    }
}

void ClientServerBridge::setEnergyMeasurementConfig(std::uint32_t everySteps,
                                                    std::uint32_t sampleLimit)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const std::uint32_t safeEvery = std::max<std::uint32_t>(1u, everySteps);
    const std::uint32_t safeSampleLimit = std::max<std::uint32_t>(2u, sampleLimit);
    sendOrQueueRemote(std::string(bltzr_protocol::SetEnergyMeasure),
                      "\"every_steps\":" + std::to_string(safeEvery) +
                          ",\"sample_limit\":" + std::to_string(safeSampleLimit));
}

void ClientServerBridge::setGpuTelemetryEnabled(bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetGpuTelemetry),
                      std::string("\"value\":") + (enabled ? "true" : "false"));
}

void ClientServerBridge::setExportDefaults(const std::string& directory, const std::string& format)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    static_cast<void>(directory);
    _defaultExportFormat = format;
}

void ClientServerBridge::setInitialStateFile(const std::string& path, const std::string& format)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!path.empty()) {
        sendOrQueueRemote(std::string(bltzr_protocol::Load),
                          "\"path\":\"" + jsonEscape(path) + "\",\"format\":\"" +
                              jsonEscape(format.empty() ? "auto" : format) + "\"");
    }
}

void ClientServerBridge::requestExportSnapshot(const std::string& outputPath,
                                               const std::string& format)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
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
    sendOrQueueRemote(std::string(bltzr_protocol::Export), fields);
}

void ClientServerBridge::requestSaveCheckpoint(const std::string& outputPath)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SaveCheckpoint),
                      "\"path\":\"" + jsonEscape(outputPath) + "\"");
}

void ClientServerBridge::requestLoadCheckpoint(const std::string& inputPath)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::LoadCheckpoint),
                      "\"path\":\"" + jsonEscape(inputPath) + "\"");
}

void ClientServerBridge::requestShutdown()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::Shutdown));
}

void ClientServerBridge::configureRemoteConnector(const std::string& host, std::uint16_t port,
                                                  bool autoStart,
                                                  const std::string& serverExecutable)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
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
    _runtimeState.setConnected(false);
    _remoteLaunchAttempted = false;
    _runtimeState.setServerLaunched(false);
    _runtimeState.clearPendingCommands();
    _reconnectRetryDelay = kReconnectRetryIntervalMin;
    _lastReconnectAttempt = Clock::time_point::min();
    _lastReconnectErrorLog = Clock::time_point::min();
    _lastRemoteErrorLog = Clock::time_point::min();
    ensureRemoteConnected(true);
}

bool ClientServerBridge::tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot,
                                            std::size_t* outSourceSize)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!ensureRemoteConnected(false)) {
        return false;
    }
    std::vector<RenderParticle> remoteSnapshot;
    std::size_t sourceSize = 0u;
    SocketTimeoutScope timeoutScope(_remoteClient, static_cast<int>(_remoteSnapshotTimeoutMs));
    const ServerClientResponse response =
        _remoteClient.getSnapshot(remoteSnapshot, _runtimeState.remoteSnapshotCap(), &sourceSize);
    if (!response.ok) {
        if (isTransportClientFailure(response.error)) {
            markRemoteDisconnected("get_snapshot", response.error);
        }
        else {
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
    if (outSourceSize != nullptr) {
        *outSourceSize = sourceSize;
    }
    outSnapshot = std::move(remoteSnapshot);
    return true;
}

SimulationStats ClientServerBridge::getStats()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    refreshRemoteStats();
    return _cachedStats;
}

void ClientServerBridge::setRemoteSnapshotCap(std::uint32_t maxPoints)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _runtimeState.setRemoteSnapshotCap(maxPoints);
    const std::uint32_t clamped = bltzr_protocol::clampSnapshotPoints(maxPoints);
    (void)sendOrQueueRemote(std::string(bltzr_protocol::SetSnapshotTransferCap),
                            "\"max_points\":" + std::to_string(clamped));
}

void ClientServerBridge::requestReconnect()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _remoteClient.disconnect();
    _runtimeState.setConnected(false);
    if (_remoteAutoStart) {
        _remoteLaunchAttempted = false;
    }
    _runtimeState.setServerLaunched(false);
    _runtimeState.clearPendingCommands();
    _reconnectRetryDelay = kReconnectRetryIntervalMin;
    _lastReconnectAttempt = Clock::time_point::min();
    ensureRemoteConnected(true);
}

bool ClientServerBridge::isRemoteMode() const
{
    return true;
}

bool ClientServerBridge::launchedByClient() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _runtimeState.serverLaunched();
}

ClientLinkState ClientServerBridge::linkState() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_runtimeState.isConnected() && _remoteClient.isConnected()) {
        return ClientLinkState::Connected;
    }
    return ClientLinkState::Reconnecting;
}

std::string_view ClientServerBridge::linkStateLabel() const
{
    const ClientLinkState state = linkState();
    switch (state) {
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
    static thread_local std::string label;
    label = _runtimeState.serverOwnerLabel();
    return label;
}

std::string ClientServerBridge::jsonEscape(const std::string& value)
{
    return bltzr_protocol::ServerJsonCodec::escapeString(value);
}

SimulationStats ClientServerBridge::fromRemoteStatus(const ServerClientStatus& status)
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

bool ClientServerBridge::sendRemoteNow(const std::string& cmd, const std::string& fields)
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

bool ClientServerBridge::sendOrQueueRemote(const std::string& cmd, const std::string& fields)
{
    if (!_runtimeState.isConnected() || !_remoteClient.isConnected()) {
        queuePendingRemoteCommand(cmd, fields);
        return true;
    }
    if (sendRemoteNow(cmd, fields)) {
        return true;
    }
    queuePendingRemoteCommand(cmd, fields);
    return true;
}

void ClientServerBridge::queuePendingRemoteCommand(const std::string& cmd,
                                                   const std::string& fields)
{
    if (_runtimeState.queuePendingCommand(cmd, fields)) {
        std::cerr << "[client] remote queue full; dropping oldest queued command\n";
    }
}

bool ClientServerBridge::ensureRemoteConnected(bool forceLog)
{
    if (_runtimeState.isConnected() && !_remoteClient.isConnected()) {
        _runtimeState.setConnected(false);
    }
    if (_runtimeState.isConnected()) {
        return true;
    }
    const auto now = Clock::now();
    if (!forceLog && (now - _lastReconnectAttempt) < _reconnectRetryDelay) {
        return false;
    }
    _lastReconnectAttempt = now;
    if (_remoteClient.connect(_remoteHost, _remotePort)) {
        _runtimeState.setConnected(true);
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
        std::cerr << "[client] server unreachable " << _remoteHost << ":" << _remotePort
                  << " (retrying)\n";
        _lastReconnectErrorLog = now;
    }
    return false;
}

void ClientServerBridge::markRemoteDisconnected(const std::string& context,
                                                const std::string& reason)
{
    const auto now = Clock::now();
    const bool logThisError =
        (reason != "not connected") || ((now - _lastRemoteErrorLog) >= kErrorLogInterval);
    if (logThisError) {
        std::cerr << "[client] remote " << context << " failed: " << reason << " (reconnecting)\n";
        _lastRemoteErrorLog = now;
    }
    _remoteClient.disconnect();
    _runtimeState.setConnected(false);
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
    return _remoteAutoStart && !_remoteLaunchAttempted && isLoopbackHost(_remoteHost);
}

void ClientServerBridge::tryAutoStartRemoteServer()
{
    if (!shouldAutoStartRemoteServer()) {
        return;
    }
    _remoteLaunchAttempted = true;
    const std::string exe =
        _serverExecutable.empty() ? std::string(kServerDefaultName) : _serverExecutable;
    const std::vector<std::string> args = {"--config",      _configPath,
                                           "--server-host", _remoteHost,
                                           "--server-port", std::to_string(_remotePort)};
    std::vector<std::string> effectiveArgs = args;
    if (!_remoteAuthToken.empty()) {
        effectiveArgs.push_back("--server-token");
        effectiveArgs.push_back(_remoteAuthToken);
    }
    std::string launchError;
    if (bltzr_platform::launchDetachedProcess(exe, effectiveArgs, launchError)) {
        _runtimeState.setServerLaunched(true);
        std::cout << "[client] auto-start server: " << exe << " (" << _remoteHost << ":"
                  << _remotePort << ")\n";
    }
    else {
        std::cerr << "[client] server auto-start failed ("
                  << (launchError.empty() ? "unknown error" : launchError)
                  << "), run manually: " << exe << " --server-host " << _remoteHost
                  << " --server-port " << _remotePort << "\n";
    }
}

void ClientServerBridge::flushPendingRemoteCommands()
{
    std::size_t sentCount = 0u;
    const std::size_t pendingCount = _runtimeState.pendingCommandCount();
    for (; sentCount < pendingCount; ++sentCount) {
        const std::pair<std::string, std::string> command =
            _runtimeState.pendingCommandAt(sentCount);
        if (!sendRemoteNow(command.first, command.second)) {
            break;
        }
    }
    _runtimeState.erasePendingPrefix(sentCount);
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
        }
        else {
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
} // namespace bltzr_client
