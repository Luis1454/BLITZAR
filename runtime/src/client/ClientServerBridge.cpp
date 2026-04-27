// File: runtime/src/client/ClientServerBridge.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

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
const std::string_view kServerDefaultName = grav_platform::serverDefaultExecutableName();
/// Description: Executes the parsePortValue operation.
bool parsePortValue(std::string_view raw, std::uint16_t& outPort)
{
    unsigned int parsed = 0u;
    if (!grav_text::parseNumber(raw, parsed) || parsed == 0u || parsed > 65535u) {
        return false;
    }
    outPort = static_cast<std::uint16_t>(parsed);
    return true;
}
/// Description: Executes the parseBoolArg operation.
bool parseBoolArg(std::string_view raw, bool& out)
{
    /// Description: Executes the normalized operation.
    std::string normalized(raw);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
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
/// Description: Executes the isTransportClientFailure operation.
bool isTransportClientFailure(std::string_view reason)
{
    return reason == "not connected" || reason == "send failed" || reason == "recv failed" ||
           reason == "invalid response";
}
/// Description: Executes the deriveDefaultServerExecutable operation.
std::string deriveDefaultServerExecutable(const std::vector<std::string_view>& rawArgs)
{
    if (rawArgs.empty() || rawArgs[0].empty()) {
        return std::string(kServerDefaultName);
    }
    std::error_code ec;
    /// Description: Executes the executablePath operation.
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
/// Description: Defines the SocketTimeoutScope data or behavior contract.
class SocketTimeoutScope {
public:
    /// Description: Executes the SocketTimeoutScope operation.
    SocketTimeoutScope(ServerClient& client, int timeoutMs)
        : _client(client), _previousTimeoutMs(client.socketTimeoutMs())
    {
        _client.setSocketTimeoutMs(timeoutMs);
    }
    /// Description: Releases resources owned by SocketTimeoutScope.
    ~SocketTimeoutScope()
    {
        _client.setSocketTimeoutMs(_previousTimeoutMs);
    }

private:
    ServerClient& _client;
    int _previousTimeoutMs;
};
/// Description: Executes the clampClientRemoteTimeoutMs operation.
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
        /// Description: Executes the raw operation.
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
      /// Description: Executes the _serverExecutable operation.
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
      /// Description: Executes the _remoteSnapshotTimeoutMs operation.
      _remoteSnapshotTimeoutMs(clampClientRemoteTimeoutMs(remoteSnapshotTimeoutMs))
{
    _remoteClient.setSocketTimeoutMs(static_cast<int>(_remoteCommandTimeoutMs));
    _remoteClient.setAuthToken(_remoteAuthToken);
}
/// Description: Executes the start operation.
bool ClientServerBridge::start()
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_runtimeState.isConnected() && _remoteClient.isConnected()) {
        return true;
    }
    if (!ensureRemoteConnected(true)) {
        return false;
    }
    /// Description: Executes the refreshRemoteStats operation.
    refreshRemoteStats();
    return _runtimeState.isConnected() && _remoteClient.isConnected();
}
/// Description: Executes the stop operation.
void ClientServerBridge::stop()
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_runtimeState.serverLaunched() && _remoteClient.isConnected()) {
        (void)_remoteClient.sendCommand(std::string(grav_protocol::Shutdown));
    }
    if (_runtimeState.isConnected()) {
        _remoteClient.disconnect();
    }
    _runtimeState.setConnected(false);
    _runtimeState.setServerLaunched(false);
}
/// Description: Executes the setPaused operation.
void ClientServerBridge::setPaused(bool paused)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    /// Description: Executes the sendOrQueueRemote operation.
    sendOrQueueRemote(std::string(paused ? grav_protocol::Pause : grav_protocol::Resume));
}
/// Description: Executes the togglePaused operation.
void ClientServerBridge::togglePaused()
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    /// Description: Executes the sendOrQueueRemote operation.
    sendOrQueueRemote(std::string(grav_protocol::Toggle));
}
/// Description: Executes the stepOnce operation.
void ClientServerBridge::stepOnce()
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    /// Description: Executes the sendOrQueueRemote operation.
    sendOrQueueRemote(std::string(grav_protocol::Step), "\"count\":1");
}
/// Description: Executes the setParticleCount operation.
void ClientServerBridge::setParticleCount(std::uint32_t particleCount)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const std::uint32_t clamped = std::max<std::uint32_t>(2u, particleCount);
    sendOrQueueRemote(std::string(grav_protocol::SetParticleCount),
                      "\"value\":" + std::to_string(clamped));
}
/// Description: Executes the setDt operation.
void ClientServerBridge::setDt(float dt)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const float clamped = std::max(1e-6f, dt);
    /// Description: Executes the sendOrQueueRemote operation.
    sendOrQueueRemote(std::string(grav_protocol::SetDt), "\"value\":" + std::to_string(clamped));
}
/// Description: Executes the scaleDt operation.
void ClientServerBridge::scaleDt(float factor)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const float currentDt = std::max(1e-6f, getStats().dt);
    const float scaled = std::max(1e-6f, currentDt * factor);
    /// Description: Executes the setDt operation.
    setDt(scaled);
}
/// Description: Executes the requestReset operation.
void ClientServerBridge::requestReset()
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    /// Description: Executes the sendOrQueueRemote operation.
    sendOrQueueRemote(std::string(grav_protocol::Reset));
}
/// Description: Executes the requestRecover operation.
void ClientServerBridge::requestRecover()
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    /// Description: Executes the sendOrQueueRemote operation.
    sendOrQueueRemote(std::string(grav_protocol::Recover));
}
/// Description: Executes the setSolverMode operation.
void ClientServerBridge::setSolverMode(const std::string& mode)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(grav_protocol::SetSolver),
                      "\"value\":\"" + jsonEscape(mode) + "\"");
}
/// Description: Executes the setIntegratorMode operation.
void ClientServerBridge::setIntegratorMode(const std::string& mode)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(grav_protocol::SetIntegrator),
                      "\"value\":\"" + jsonEscape(mode) + "\"");
}
/// Description: Executes the setPerformanceProfile operation.
void ClientServerBridge::setPerformanceProfile(const std::string& profile)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(grav_protocol::SetPerformanceProfile),
                      "\"value\":\"" + jsonEscape(profile) + "\"");
}
/// Description: Executes the setOctreeParameters operation.
void ClientServerBridge::setOctreeParameters(float theta, float softening)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const float safeTheta = std::max(0.0001f, theta);
    const float safeSoftening = std::max(0.000001f, softening);
    sendOrQueueRemote(std::string(grav_protocol::SetOctree),
                      "\"theta\":" + std::to_string(safeTheta) +
                          ",\"softening\":" + std::to_string(safeSoftening));
}
/// Description: Executes the setSphEnabled operation.
void ClientServerBridge::setSphEnabled(bool enabled)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(grav_protocol::SetSph),
                      /// Description: Executes the string operation.
                      std::string("\"value\":") + (enabled ? "true" : "false"));
}
void ClientServerBridge::setSphParameters(float smoothingLength, float restDensity,
                                          float gasConstant, float viscosity)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const float safeH = std::max(0.000001f, smoothingLength);
    const float safeRestDensity = std::max(0.000001f, restDensity);
    const float safeGasConstant = std::max(0.000001f, gasConstant);
    const float safeViscosity = std::max(0.0f, viscosity);
    sendOrQueueRemote(std::string(grav_protocol::SetSphParams),
                      "\"h\":" + std::to_string(safeH) +
                          ",\"rest_density\":" + std::to_string(safeRestDensity) +
                          ",\"gas_constant\":" + std::to_string(safeGasConstant) +
                          ",\"viscosity\":" + std::to_string(safeViscosity));
}
/// Description: Executes the setSubstepPolicy operation.
void ClientServerBridge::setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const float safeTargetDt = std::max(0.0f, targetDt);
    const std::uint32_t safeMaxSubsteps = std::max<std::uint32_t>(1u, maxSubsteps);
    sendOrQueueRemote(std::string(grav_protocol::SetSubsteps),
                      "\"target_dt\":" + std::to_string(safeTargetDt) +
                          ",\"max_substeps\":" + std::to_string(safeMaxSubsteps));
}
/// Description: Executes the setSnapshotPublishPeriodMs operation.
void ClientServerBridge::setSnapshotPublishPeriodMs(std::uint32_t periodMs)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const std::uint32_t safePeriodMs = std::max<std::uint32_t>(1u, periodMs);
    sendOrQueueRemote(std::string(grav_protocol::SetSnapshotPublishCadence),
                      "\"period_ms\":" + std::to_string(safePeriodMs));
}
/// Description: Executes the setInitialStateConfig operation.
void ClientServerBridge::setInitialStateConfig(const InitialStateConfig& config)
{
    /// Description: Executes the lock operation.
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
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const std::uint32_t safeEvery = std::max<std::uint32_t>(1u, everySteps);
    const std::uint32_t safeSampleLimit = std::max<std::uint32_t>(2u, sampleLimit);
    sendOrQueueRemote(std::string(grav_protocol::SetEnergyMeasure),
                      "\"every_steps\":" + std::to_string(safeEvery) +
                          ",\"sample_limit\":" + std::to_string(safeSampleLimit));
}
/// Description: Executes the setGpuTelemetryEnabled operation.
void ClientServerBridge::setGpuTelemetryEnabled(bool enabled)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(grav_protocol::SetGpuTelemetry),
                      /// Description: Executes the string operation.
                      std::string("\"value\":") + (enabled ? "true" : "false"));
}
/// Description: Executes the setExportDefaults operation.
void ClientServerBridge::setExportDefaults(const std::string& directory, const std::string& format)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    static_cast<void>(directory);
    _defaultExportFormat = format;
}
/// Description: Executes the setInitialStateFile operation.
void ClientServerBridge::setInitialStateFile(const std::string& path, const std::string& format)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!path.empty()) {
        sendOrQueueRemote(std::string(grav_protocol::Load),
                          "\"path\":\"" + jsonEscape(path) + "\",\"format\":\"" +
                              /// Description: Executes the jsonEscape operation.
                              jsonEscape(format.empty() ? "auto" : format) + "\"");
    }
}
void ClientServerBridge::requestExportSnapshot(const std::string& outputPath,
                                               const std::string& format)
{
    /// Description: Executes the lock operation.
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
    /// Description: Executes the sendOrQueueRemote operation.
    sendOrQueueRemote(std::string(grav_protocol::Export), fields);
}
/// Description: Executes the requestSaveCheckpoint operation.
void ClientServerBridge::requestSaveCheckpoint(const std::string& outputPath)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(grav_protocol::SaveCheckpoint),
                      "\"path\":\"" + jsonEscape(outputPath) + "\"");
}
/// Description: Executes the requestLoadCheckpoint operation.
void ClientServerBridge::requestLoadCheckpoint(const std::string& inputPath)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(grav_protocol::LoadCheckpoint),
                      "\"path\":\"" + jsonEscape(inputPath) + "\"");
}
/// Description: Executes the requestShutdown operation.
void ClientServerBridge::requestShutdown()
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    /// Description: Executes the sendOrQueueRemote operation.
    sendOrQueueRemote(std::string(grav_protocol::Shutdown));
}
void ClientServerBridge::configureRemoteConnector(const std::string& host, std::uint16_t port,
                                                  bool autoStart,
                                                  const std::string& serverExecutable)
{
    /// Description: Executes the lock operation.
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
    /// Description: Executes the ensureRemoteConnected operation.
    ensureRemoteConnected(true);
}
bool ClientServerBridge::tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot,
                                            std::size_t* outSourceSize)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!ensureRemoteConnected(false)) {
        return false;
    }
    std::vector<RenderParticle> remoteSnapshot;
    std::size_t sourceSize = 0u;
    /// Description: Executes the timeoutScope operation.
    SocketTimeoutScope timeoutScope(_remoteClient, static_cast<int>(_remoteSnapshotTimeoutMs));
    const ServerClientResponse response =
        _remoteClient.getSnapshot(remoteSnapshot, _runtimeState.remoteSnapshotCap(), &sourceSize);
    if (!response.ok) {
        if (isTransportClientFailure(response.error)) {
            /// Description: Executes the markRemoteDisconnected operation.
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
/// Description: Executes the getStats operation.
SimulationStats ClientServerBridge::getStats()
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    /// Description: Executes the refreshRemoteStats operation.
    refreshRemoteStats();
    return _cachedStats;
}
/// Description: Executes the setRemoteSnapshotCap operation.
void ClientServerBridge::setRemoteSnapshotCap(std::uint32_t maxPoints)
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _runtimeState.setRemoteSnapshotCap(maxPoints);
    const std::uint32_t clamped = grav_protocol::clampSnapshotPoints(maxPoints);
    (void)sendOrQueueRemote(std::string(grav_protocol::SetSnapshotTransferCap),
                            "\"max_points\":" + std::to_string(clamped));
}
/// Description: Executes the requestReconnect operation.
void ClientServerBridge::requestReconnect()
{
    /// Description: Executes the lock operation.
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
    /// Description: Executes the ensureRemoteConnected operation.
    ensureRemoteConnected(true);
}
/// Description: Executes the isRemoteMode operation.
bool ClientServerBridge::isRemoteMode() const
{
    return true;
}
/// Description: Executes the launchedByClient operation.
bool ClientServerBridge::launchedByClient() const
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _runtimeState.serverLaunched();
}
/// Description: Executes the linkState operation.
ClientLinkState ClientServerBridge::linkState() const
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_runtimeState.isConnected() && _remoteClient.isConnected()) {
        return ClientLinkState::Connected;
    }
    return ClientLinkState::Reconnecting;
}
/// Description: Executes the linkStateLabel operation.
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
/// Description: Executes the serverOwnerLabel operation.
std::string_view ClientServerBridge::serverOwnerLabel() const
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    static thread_local std::string label;
    label = _runtimeState.serverOwnerLabel();
    return label;
}
/// Description: Executes the jsonEscape operation.
std::string ClientServerBridge::jsonEscape(const std::string& value)
{
    return grav_protocol::ServerJsonCodec::escapeString(value);
}
/// Description: Executes the fromRemoteStatus operation.
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
/// Description: Executes the sendRemoteNow operation.
bool ClientServerBridge::sendRemoteNow(const std::string& cmd, const std::string& fields)
{
    if (!ensureRemoteConnected(false)) {
        return false;
    }
    /// Description: Executes the timeoutScope operation.
    SocketTimeoutScope timeoutScope(_remoteClient, static_cast<int>(_remoteCommandTimeoutMs));
    const ServerClientResponse response = _remoteClient.sendCommand(cmd, fields);
    if (!response.ok) {
        if (isTransportClientFailure(response.error)) {
            /// Description: Executes the markRemoteDisconnected operation.
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
/// Description: Executes the sendOrQueueRemote operation.
bool ClientServerBridge::sendOrQueueRemote(const std::string& cmd, const std::string& fields)
{
    if (!_runtimeState.isConnected() || !_remoteClient.isConnected()) {
        /// Description: Executes the queuePendingRemoteCommand operation.
        queuePendingRemoteCommand(cmd, fields);
        return true;
    }
    if (sendRemoteNow(cmd, fields)) {
        return true;
    }
    /// Description: Executes the queuePendingRemoteCommand operation.
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
/// Description: Executes the ensureRemoteConnected operation.
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
        /// Description: Executes the flushPendingRemoteCommands operation.
        flushPendingRemoteCommands();
        return true;
    }
    if (_reconnectRetryDelay < kReconnectRetryIntervalMax) {
        const auto grown = _reconnectRetryDelay * 2;
        _reconnectRetryDelay = std::min(grown, kReconnectRetryIntervalMax);
    }
    /// Description: Executes the tryAutoStartRemoteServer operation.
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
/// Description: Executes the isLoopbackHost operation.
bool ClientServerBridge::isLoopbackHost(std::string_view host)
{
    if (host.empty()) {
        return true;
    }
    return host == "127.0.0.1" || host == "localhost";
}
/// Description: Executes the shouldAutoStartRemoteServer operation.
bool ClientServerBridge::shouldAutoStartRemoteServer() const
{
    return _remoteAutoStart && !_remoteLaunchAttempted && isLoopbackHost(_remoteHost);
}
/// Description: Executes the tryAutoStartRemoteServer operation.
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
    if (grav_platform::launchDetachedProcess(exe, effectiveArgs, launchError)) {
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
/// Description: Executes the flushPendingRemoteCommands operation.
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
/// Description: Executes the refreshRemoteStats operation.
void ClientServerBridge::refreshRemoteStats()
{
    if (!ensureRemoteConnected(false)) {
        return;
    }
    ServerClientStatus status{};
    /// Description: Executes the timeoutScope operation.
    SocketTimeoutScope timeoutScope(_remoteClient, static_cast<int>(_remoteStatusTimeoutMs));
    const ServerClientResponse response = _remoteClient.getStatus(status);
    if (!response.ok) {
        if (isTransportClientFailure(response.error)) {
            /// Description: Executes the markRemoteDisconnected operation.
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
} // namespace grav_client
