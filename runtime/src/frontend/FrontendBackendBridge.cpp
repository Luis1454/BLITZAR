#include "frontend/FrontendBackendBridge.hpp"

#include "protocol/BackendJsonCodec.hpp"
#include "protocol/BackendProtocol.hpp"
#include "platform/PlatformPaths.hpp"
#include "platform/PlatformProcess.hpp"
#include "config/TextParse.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <iostream>

namespace grav_frontend {

const std::uint32_t kFrontendRemoteTimeoutMinMs = 10u;
const std::uint32_t kFrontendRemoteTimeoutMaxMs = 60000u;
const std::uint32_t kFrontendRemoteCommandTimeoutMsDefault = 80u;
const std::uint32_t kFrontendRemoteStatusTimeoutMsDefault = 40u;
const std::uint32_t kFrontendRemoteSnapshotTimeoutMsDefault = 140u;

typedef std::chrono::steady_clock Clock;
constexpr auto kReconnectRetryIntervalMin = std::chrono::milliseconds(50);
constexpr auto kReconnectRetryIntervalMax = std::chrono::milliseconds(1000);
constexpr auto kErrorLogInterval = std::chrono::milliseconds(1500);
constexpr std::size_t kPendingRemoteCommandsMax = 256u;

const std::string_view kBackendDefaultName = grav_platform::backendDefaultExecutableName();

bool parsePortValue(std::string_view raw, std::uint16_t &outPort)
{
    unsigned int parsed = 0u;

    if (!grav_text::parseNumber(raw, parsed) || parsed == 0u || parsed > 65535u)
        return false;

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
    return reason == "not connected" || reason == "send failed"
        || reason == "recv failed" || reason == "invalid response";
}

std::string deriveDefaultBackendExecutable(const std::vector<std::string_view> &rawArgs)
{
    if (rawArgs.empty() || rawArgs[0].empty())
        return std::string(kBackendDefaultName);

    std::error_code ec;
    const std::filesystem::path executablePath(rawArgs[0]);
    const std::filesystem::path dir = executablePath.parent_path();
    if (!dir.empty()) {
        const std::filesystem::path candidate = dir / std::string(kBackendDefaultName);
        if (std::filesystem::exists(candidate, ec) && !ec)
            return candidate.string();
    }
    return std::string(kBackendDefaultName);
}

class SocketTimeoutScope {
    public:
        SocketTimeoutScope(BackendClient &client, int timeoutMs)
        : _client(client), _previousTimeoutMs(client.socketTimeoutMs()) {
            _client.setSocketTimeoutMs(timeoutMs);
        }

        ~SocketTimeoutScope() {
            _client.setSocketTimeoutMs(_previousTimeoutMs);
        }

    private:
        BackendClient &_client;
        int _previousTimeoutMs;
};

template <typename LocalCall, typename RemoteCall>
void dispatchBackendCall(bool remoteMode, const std::unique_ptr<ILocalBackend> &localBackend,
                        LocalCall &&localCall, RemoteCall &&remoteCall) {
    if (!remoteMode) {
        if (localBackend)
            localCall(*localBackend);
        return;
    }
    remoteCall();
}

std::uint32_t clampFrontendRemoteTimeoutMs(std::uint32_t timeoutMs) {
    return std::clamp(timeoutMs, kFrontendRemoteTimeoutMinMs, kFrontendRemoteTimeoutMaxMs);
}

bool splitFrontendTransportArgs(
    const std::vector<std::string_view> &rawArgs,
    std::vector<std::string_view> &filteredArgs,
    FrontendTransportArgs &transport,
    std::ostream &warnings)
{
    if (transport.backendExecutable.empty()) {
        transport.backendExecutable = deriveDefaultBackendExecutable(rawArgs);
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
        if (raw == "--backend-host") {
            if (i + 1 >= rawArgs.size()) {
                warnings << "[args] missing value for --backend-host\n";
                return false;
            }
            transport.remoteHost = std::string(rawArgs[++i]);
            transport.remoteMode = true;
            continue;
        }
        if (raw.rfind("--backend-host=", 0) == 0) {
            transport.remoteHost = raw.substr(std::string("--backend-host=").size());
            transport.remoteMode = true;
            continue;
        }
        if (raw == "--backend-port") {
            if (i + 1 >= rawArgs.size()) {
                warnings << "[args] missing value for --backend-port\n";
                return false;
            }
            std::uint16_t parsedPort = transport.remotePort;
            if (!parsePortValue(rawArgs[++i], parsedPort)) {
                warnings << "[args] invalid --backend-port value\n";
                return false;
            }
            transport.remotePort = parsedPort;
            transport.remoteMode = true;
            continue;
        }
        if (raw.rfind("--backend-port=", 0) == 0) {
            std::uint16_t parsedPort = transport.remotePort;
            const std::string value = raw.substr(std::string("--backend-port=").size());
            if (!parsePortValue(value, parsedPort)) {
                warnings << "[args] invalid --backend-port value: " << value << "\n";
                return false;
            }
            transport.remotePort = parsedPort;
            transport.remoteMode = true;
            continue;
        }
        if (raw == "--backend-autostart") {
            if (i + 1 < rawArgs.size()) {
                bool parsed = transport.remoteAutoStart;
                if (parseBoolArg(rawArgs[i + 1], parsed)) {
                    transport.remoteAutoStart = parsed;
                    ++i;
                }
            }
            continue;
        }
        if (raw.rfind("--backend-autostart=", 0) == 0) {
            bool parsed = transport.remoteAutoStart;
            const std::string value = raw.substr(std::string("--backend-autostart=").size());
            if (!parseBoolArg(value, parsed)) {
                warnings << "[args] invalid --backend-autostart value: " << value << "\n";
                return false;
            }
            transport.remoteAutoStart = parsed;
            continue;
        }
        if (raw == "--backend-bin") {
            if (i + 1 >= rawArgs.size()) {
                warnings << "[args] missing value for --backend-bin\n";
                return false;
            }
            transport.backendExecutable = std::string(rawArgs[++i]);
            continue;
        }
        if (raw.rfind("--backend-bin=", 0) == 0) {
            transport.backendExecutable = raw.substr(std::string("--backend-bin=").size());
            continue;
        }
        if (raw == "--backend-token") {
            if (i + 1 >= rawArgs.size()) {
                warnings << "[args] missing value for --backend-token\n";
                return false;
            }
            transport.remoteAuthToken = std::string(rawArgs[++i]);
            transport.remoteMode = true;
            continue;
        }
        if (raw.rfind("--backend-token=", 0) == 0) {
            transport.remoteAuthToken = raw.substr(std::string("--backend-token=").size());
            transport.remoteMode = true;
            continue;
        }

        filteredArgs.push_back(rawArgs[i]);
    }

    return true;
}

FrontendBackendBridge::FrontendBackendBridge(
    const std::string &configPath,
    bool remoteMode,
    std::string remoteHost,
    std::uint16_t remotePort,
    bool remoteAutoStart,
    std::string backendExecutable,
    std::string remoteAuthToken,
    std::uint32_t remoteCommandTimeoutMs,
    std::uint32_t remoteStatusTimeoutMs,
    std::uint32_t remoteSnapshotTimeoutMs,
    LocalBackendFactory localBackendFactory)
    : _configPath(configPath),
      _remoteMode(remoteMode),
      _localBackendFactory(std::move(localBackendFactory)),
      _remoteHost(std::move(remoteHost)),
      _remotePort(remotePort),
      _remoteAutoStart(remoteAutoStart),
      _backendExecutable(backendExecutable.empty() ? std::string(kBackendDefaultName) : std::move(backendExecutable)),
      _remoteAuthToken(std::move(remoteAuthToken)),
      _localBackend(nullptr),
      _remoteClient(),
      _remoteConnected(false),
      _remoteLaunchAttempted(false),
      _remoteBackendLaunched(false),
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
      _remoteCommandTimeoutMs(clampFrontendRemoteTimeoutMs(remoteCommandTimeoutMs)),
      _remoteStatusTimeoutMs(clampFrontendRemoteTimeoutMs(remoteStatusTimeoutMs)),
      _remoteSnapshotTimeoutMs(clampFrontendRemoteTimeoutMs(remoteSnapshotTimeoutMs))
{
    if (!_remoteMode && _localBackendFactory) {
        _localBackend = _localBackendFactory(configPath);
    }
    if (_remoteMode) {
        _remoteClient.setSocketTimeoutMs(static_cast<int>(_remoteCommandTimeoutMs));
        _remoteClient.setAuthToken(_remoteAuthToken);
    }
}

bool FrontendBackendBridge::start()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!_remoteMode) {
        if (_localBackend == nullptr && _localBackendFactory) {
            _localBackend = _localBackendFactory(_configPath);
        }
        if (_localBackend != nullptr) {
            _localBackend->start();
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

void FrontendBackendBridge::stop()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_remoteMode) {
        if (_remoteBackendLaunched && _remoteClient.isConnected()) {
            (void)_remoteClient.sendCommand(std::string(grav_protocol::Shutdown));
        }
        if (_remoteConnected) {
            _remoteClient.disconnect();
        }
        _remoteConnected = false;
        _remoteBackendLaunched = false;
        return;
    }
    if (_localBackend != nullptr) {
        _localBackend->stop();
    }
}

void FrontendBackendBridge::setPaused(bool paused)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [paused](ILocalBackend &backend) { backend.setPaused(paused); },
        [this, paused]() {
            sendOrQueueRemote(std::string(paused ? grav_protocol::Pause : grav_protocol::Resume));
        });
}

void FrontendBackendBridge::togglePaused()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [](ILocalBackend &backend) { backend.togglePaused(); },
        [this]() { sendOrQueueRemote(std::string(grav_protocol::Toggle)); });
}

void FrontendBackendBridge::stepOnce()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [](ILocalBackend &backend) { backend.stepOnce(); },
        [this]() { sendOrQueueRemote(std::string(grav_protocol::Step), "\"count\":1"); });
}

void FrontendBackendBridge::setParticleCount(std::uint32_t particleCount)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [particleCount](ILocalBackend &backend) { backend.setParticleCount(particleCount); },
        [this, particleCount]() {
            const std::uint32_t clamped = std::max<std::uint32_t>(2u, particleCount);
            sendOrQueueRemote(std::string(grav_protocol::SetParticleCount), "\"value\":" + std::to_string(clamped));
        });
}

void FrontendBackendBridge::setDt(float dt)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [dt](ILocalBackend &backend) { backend.setDt(dt); },
        [this, dt]() {
            const float clamped = std::max(1e-6f, dt);
            sendOrQueueRemote(std::string(grav_protocol::SetDt), "\"value\":" + std::to_string(clamped));
        });
}

void FrontendBackendBridge::scaleDt(float factor)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [factor](ILocalBackend &backend) { backend.scaleDt(factor); },
        [this, factor]() {
            const float currentDt = std::max(1e-6f, getStats().dt);
            const float scaled = std::max(1e-6f, currentDt * factor);
            setDt(scaled);
        });
}

void FrontendBackendBridge::requestReset()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [](ILocalBackend &backend) { backend.requestReset(); },
        [this]() { sendOrQueueRemote(std::string(grav_protocol::Reset)); });
}

void FrontendBackendBridge::requestRecover()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [](ILocalBackend &backend) { backend.requestReset(); },
        [this]() { sendOrQueueRemote(std::string(grav_protocol::Recover)); });
}

void FrontendBackendBridge::setSolverMode(const std::string &mode)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [&mode](ILocalBackend &backend) { backend.setSolverMode(mode); },
        [this, &mode]() {
            sendOrQueueRemote(std::string(grav_protocol::SetSolver), "\"value\":\"" + jsonEscape(mode) + "\"");
        });
}

void FrontendBackendBridge::setIntegratorMode(const std::string &mode)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [&mode](ILocalBackend &backend) { backend.setIntegratorMode(mode); },
        [this, &mode]() {
            sendOrQueueRemote(std::string(grav_protocol::SetIntegrator), "\"value\":\"" + jsonEscape(mode) + "\"");
        });
}

void FrontendBackendBridge::setOctreeParameters(float theta, float softening)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [theta, softening](ILocalBackend &backend) { backend.setOctreeParameters(theta, softening); },
        [this, theta, softening]() {
            const float safeTheta = std::max(0.0001f, theta);
            const float safeSoftening = std::max(0.000001f, softening);
            sendOrQueueRemote(
                std::string(grav_protocol::SetOctree),
                "\"theta\":" + std::to_string(safeTheta) + ",\"softening\":" + std::to_string(safeSoftening));
        });
}

void FrontendBackendBridge::setSphEnabled(bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [enabled](ILocalBackend &backend) { backend.setSphEnabled(enabled); },
        [this, enabled]() {
            sendOrQueueRemote(std::string(grav_protocol::SetSph), std::string("\"value\":") + (enabled ? "true" : "false"));
        });
}

void FrontendBackendBridge::setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [smoothingLength, restDensity, gasConstant, viscosity](ILocalBackend &backend) {
            backend.setSphParameters(smoothingLength, restDensity, gasConstant, viscosity);
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

void FrontendBackendBridge::setInitialStateConfig(const InitialStateConfig &config)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [&config](ILocalBackend &backend) { backend.setInitialStateConfig(config); },
        [this]() {
            if (!_warnedRemoteInitialConfig) {
                std::cout << "[frontend] remote backend: initial state config templates are backend-owned "
                             "(set/load through backend API)\n";
                _warnedRemoteInitialConfig = true;
            }
        });
}

void FrontendBackendBridge::setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [everySteps, sampleLimit](ILocalBackend &backend) { backend.setEnergyMeasurementConfig(everySteps, sampleLimit); },
        [this, everySteps, sampleLimit]() {
            const std::uint32_t safeEvery = std::max<std::uint32_t>(1u, everySteps);
            const std::uint32_t safeSampleLimit = std::max<std::uint32_t>(2u, sampleLimit);
            sendOrQueueRemote(
                std::string(grav_protocol::SetEnergyMeasure),
                "\"every_steps\":" + std::to_string(safeEvery) + ",\"sample_limit\":" + std::to_string(safeSampleLimit));
        });
}

void FrontendBackendBridge::setExportDefaults(const std::string &directory, const std::string &format)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [&directory, &format](ILocalBackend &backend) { backend.setExportDefaults(directory, format); },
        [this, &format]() { _defaultExportFormat = format; });
}

void FrontendBackendBridge::setInitialStateFile(const std::string &path, const std::string &format)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [&path, &format](ILocalBackend &backend) { backend.setInitialStateFile(path, format); },
        [this, &path, &format]() {
            if (!path.empty()) {
                sendOrQueueRemote(
                    std::string(grav_protocol::Load),
                    "\"path\":\"" + jsonEscape(path) + "\",\"format\":\"" + jsonEscape(format.empty() ? "auto" : format) + "\"");
            }
        });
}

void FrontendBackendBridge::requestExportSnapshot(const std::string &outputPath, const std::string &format)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [&outputPath, &format](ILocalBackend &backend) { backend.requestExportSnapshot(outputPath, format); },
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

void FrontendBackendBridge::requestShutdown()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    dispatchBackendCall(
        _remoteMode,
        _localBackend,
        [](ILocalBackend &backend) { backend.stop(); },
        [this]() { sendOrQueueRemote(std::string(grav_protocol::Shutdown)); });
}

void FrontendBackendBridge::configureRemoteConnector(
    const std::string &host,
    std::uint16_t port,
    bool autoStart,
    const std::string &backendExecutable)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    if (!_remoteMode) {
        if (_localBackend) {
            _localBackend->stop();
            _localBackend.reset();
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
    if (!backendExecutable.empty()) {
        _backendExecutable = backendExecutable;
    }
    _remoteClient.setAuthToken(_remoteAuthToken);

    _remoteClient.disconnect();
    _remoteConnected = false;
    _remoteLaunchAttempted = false;
    _remoteBackendLaunched = false;
    _pendingRemoteCommands.clear();
    _pendingQueueDropWarned = false;
    _reconnectRetryDelay = kReconnectRetryIntervalMin;
    _lastReconnectAttempt = Clock::time_point::min();
    _lastReconnectErrorLog = Clock::time_point::min();
    _lastRemoteErrorLog = Clock::time_point::min();

    ensureRemoteConnected(true);
}

bool FrontendBackendBridge::tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!_remoteMode) {
        return _localBackend != nullptr && _localBackend->tryConsumeSnapshot(outSnapshot);
    }
    if (!ensureRemoteConnected(false)) {
        return false;
    }
    std::vector<RenderParticle> remoteSnapshot;
    SocketTimeoutScope timeoutScope(_remoteClient, static_cast<int>(_remoteSnapshotTimeoutMs));
    const BackendClientResponse response = _remoteClient.getSnapshot(
        remoteSnapshot,
        grav_protocol::clampSnapshotPoints(_remoteSnapshotCap));
    if (!response.ok) {
        if (isTransportClientFailure(response.error)) {
            markRemoteDisconnected("get_snapshot", response.error);
        } else {
            const auto now = Clock::now();
            if ((now - _lastRemoteErrorLog) >= kErrorLogInterval) {
                std::cerr << "[frontend] remote get_snapshot rejected: " << response.error << "\n";
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

SimulationStats FrontendBackendBridge::getStats()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!_remoteMode) {
        return _localBackend != nullptr ? _localBackend->getStats() : SimulationStats{};
    }
    refreshRemoteStats();
    return _cachedStats;
}

void FrontendBackendBridge::setRemoteSnapshotCap(std::uint32_t maxPoints)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _remoteSnapshotCap = grav_protocol::clampSnapshotPoints(maxPoints);
}

void FrontendBackendBridge::requestReconnect()
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
    _remoteBackendLaunched = false;
    _pendingRemoteCommands.clear();
    _pendingQueueDropWarned = false;
    _reconnectRetryDelay = kReconnectRetryIntervalMin;
    _lastReconnectAttempt = Clock::time_point::min();
    ensureRemoteConnected(true);
}

bool FrontendBackendBridge::isRemoteMode() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _remoteMode;
}

bool FrontendBackendBridge::launchedByFrontend() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _remoteMode && _remoteBackendLaunched;
}

FrontendLinkState FrontendBackendBridge::linkState() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!_remoteMode) {
        return FrontendLinkState::LocalEmbedded;
    }
    if (_remoteConnected && _remoteClient.isConnected()) {
        return FrontendLinkState::Connected;
    }
    return FrontendLinkState::Reconnecting;
}

std::string_view FrontendBackendBridge::linkStateLabel() const
{
    const FrontendLinkState state = linkState();
    switch (state) {
        case FrontendLinkState::LocalEmbedded:
            return "local";
        case FrontendLinkState::Connected:
            return "connected";
        case FrontendLinkState::Reconnecting:
        default:
            return "reconnecting";
    }
}

std::string_view FrontendBackendBridge::backendOwnerLabel() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!_remoteMode) {
        return "embedded";
    }
    if (_remoteBackendLaunched) {
        return "managed";
    }
    return "external";
}

std::string FrontendBackendBridge::jsonEscape(const std::string &value)
{
    return grav_protocol::BackendJsonCodec::escapeString(value);
}

SimulationStats FrontendBackendBridge::fromRemoteStatus(const BackendClientStatus &status)
{
    SimulationStats stats{};
    stats.steps = status.steps;
    stats.dt = status.dt;
    stats.paused = status.paused;
    stats.faulted = status.faulted;
    stats.faultStep = status.faultStep;
    stats.faultReason = status.faultReason;
    stats.sphEnabled = status.sphEnabled;
    stats.backendFps = status.backendFps;
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

bool FrontendBackendBridge::sendRemoteNow(const std::string &cmd, const std::string &fields)
{
    if (!ensureRemoteConnected(false)) {
        return false;
    }
    SocketTimeoutScope timeoutScope(_remoteClient, static_cast<int>(_remoteCommandTimeoutMs));
    const BackendClientResponse response = _remoteClient.sendCommand(cmd, fields);
    if (!response.ok) {
        if (isTransportClientFailure(response.error)) {
            markRemoteDisconnected(cmd, response.error);
            return false;
        }
        const auto now = Clock::now();
        if ((now - _lastRemoteErrorLog) >= kErrorLogInterval) {
            std::cerr << "[frontend] remote " << cmd << " rejected: " << response.error << "\n";
            _lastRemoteErrorLog = now;
        }
        return true;
    }
    return true;
}

bool FrontendBackendBridge::sendOrQueueRemote(const std::string &cmd, const std::string &fields)
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

void FrontendBackendBridge::queuePendingRemoteCommand(const std::string &cmd, const std::string &fields)
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
            std::cerr << "[frontend] remote queue full; dropping oldest queued command\n";
            _pendingQueueDropWarned = true;
        }
    }
    _pendingRemoteCommands.emplace_back(cmd, fields);
}

bool FrontendBackendBridge::ensureRemoteConnected(bool forceLog)
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

    tryAutoStartRemoteBackend();

    if (forceLog || (now - _lastReconnectErrorLog) >= kErrorLogInterval) {
        std::cerr << "[frontend] backend unreachable " << _remoteHost << ":" << _remotePort << " (retrying)\n";
        _lastReconnectErrorLog = now;
    }
    return false;
}

void FrontendBackendBridge::markRemoteDisconnected(const std::string &context, const std::string &reason)
{
    const auto now = Clock::now();
    const bool logThisError =
        (reason != "not connected")
        || ((now - _lastRemoteErrorLog) >= kErrorLogInterval);
    if (logThisError) {
        std::cerr << "[frontend] remote " << context << " failed: " << reason << " (reconnecting)\n";
        _lastRemoteErrorLog = now;
    }
    _remoteClient.disconnect();
    _remoteConnected = false;
    _reconnectRetryDelay = kReconnectRetryIntervalMin;
    _lastReconnectAttempt = now;
}

bool FrontendBackendBridge::isLoopbackHost(std::string_view host)
{
    if (host.empty()) {
        return true;
    }
    return host == "127.0.0.1" || host == "localhost";
}

bool FrontendBackendBridge::shouldAutoStartRemoteBackend() const
{
    return _remoteMode
        && _remoteAutoStart
        && !_remoteLaunchAttempted
        && isLoopbackHost(_remoteHost);
}

void FrontendBackendBridge::tryAutoStartRemoteBackend()
{
    if (!shouldAutoStartRemoteBackend()) {
        return;
    }
    _remoteLaunchAttempted = true;
    const std::string exe = _backendExecutable.empty() ? std::string(kBackendDefaultName) : _backendExecutable;
    const std::vector<std::string> args = {
        "--config",
        _configPath,
        "--backend-host",
        _remoteHost,
        "--backend-port",
        std::to_string(_remotePort)
    };
    std::vector<std::string> effectiveArgs = args;
    if (!_remoteAuthToken.empty()) {
        effectiveArgs.push_back("--backend-token");
        effectiveArgs.push_back(_remoteAuthToken);
    }
    std::string launchError;
    if (grav_platform::launchDetachedProcess(exe, effectiveArgs, launchError)) {
        _remoteBackendLaunched = true;
        std::cout << "[frontend] auto-start backend: " << exe << " (" << _remoteHost << ":" << _remotePort << ")\n";
    } else {
        std::cerr << "[frontend] backend auto-start failed ("
                  << (launchError.empty() ? "unknown error" : launchError)
                  << "), run manually: " << exe << " --backend-host " << _remoteHost
                  << " --backend-port " << _remotePort << "\n";
    }
}

void FrontendBackendBridge::flushPendingRemoteCommands()
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

void FrontendBackendBridge::refreshRemoteStats()
{
    if (!ensureRemoteConnected(false)) {
        return;
    }
    BackendClientStatus status{};
    SocketTimeoutScope timeoutScope(_remoteClient, static_cast<int>(_remoteStatusTimeoutMs));
    const BackendClientResponse response = _remoteClient.getStatus(status);
    if (!response.ok) {
        if (isTransportClientFailure(response.error)) {
            markRemoteDisconnected("status", response.error);
        } else {
            const auto now = Clock::now();
            if ((now - _lastRemoteErrorLog) >= kErrorLogInterval) {
                std::cerr << "[frontend] remote status rejected: " << response.error << "\n";
                _lastRemoteErrorLog = now;
            }
        }
        return;
    }
    _cachedStats = fromRemoteStatus(status);
}

} // namespace grav_frontend
