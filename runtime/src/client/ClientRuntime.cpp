#include "client/ClientRuntime.hpp"
#include "client/LocalServerFactory.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>

namespace grav_client {
constexpr auto kIdleSleepInterval = std::chrono::milliseconds(2);
constexpr auto kSnapshotPollInterval = std::chrono::milliseconds(16);
constexpr auto kStatusPollInterval = std::chrono::milliseconds(120);
ClientRuntime::ClientRuntime(const std::string &configPath, const ClientTransportArgs &transport)
    : _bridge(
          configPath,
          transport.remoteMode,
          transport.remoteHost,
          transport.remotePort,
          transport.remoteAutoStart,
          transport.serverExecutable,
          transport.remoteAuthToken,
          transport.remoteCommandTimeoutMs,
          transport.remoteStatusTimeoutMs,
          transport.remoteSnapshotTimeoutMs,
          createLocalServer),
      _pollThread(),
      _pollRunning(false),
      _dataMutex(),
      _latestStats(),
      _latestSnapshot(),
      _hasNewSnapshot(false),
      _latestSnapshotSize(0u),
      _latestLinkLabel(transport.remoteMode ? "reconnecting" : "local"),
      _latestOwnerLabel(transport.remoteMode ? "external" : "embedded"),
      _lastStatsAt(),
      _lastSnapshotAt(),
      _hasStats(false),
      _hasSnapshotEver(false)
{
}

ClientRuntime::~ClientRuntime()
{
    stop();
}

bool ClientRuntime::start()
{
    stop();
    if (!_bridge.start()) {
        return false;
    }
    pollOnce(true, true);
    _pollRunning.store(true);
    _pollThread = std::thread([this]() { pollLoop(); });
    return true;
}

void ClientRuntime::stop()
{
    _pollRunning.store(false);
    if (_pollThread.joinable()) {
        _pollThread.join();
    }
    _bridge.stop();
}

void ClientRuntime::setPaused(bool paused)
{
    _bridge.setPaused(paused);
}

void ClientRuntime::togglePaused()
{
    _bridge.togglePaused();
}

void ClientRuntime::stepOnce()
{
    _bridge.stepOnce();
}

void ClientRuntime::setParticleCount(std::uint32_t particleCount)
{
    _bridge.setParticleCount(particleCount);
}

void ClientRuntime::setDt(float dt)
{
    _bridge.setDt(dt);
}

void ClientRuntime::scaleDt(float factor)
{
    _bridge.scaleDt(factor);
}

void ClientRuntime::requestReset()
{
    _bridge.requestReset();
    invalidateCachedSnapshot();
}

void ClientRuntime::requestRecover()
{
    _bridge.requestRecover();
    invalidateCachedSnapshot();
}

void ClientRuntime::setSolverMode(const std::string &mode)
{
    _bridge.setSolverMode(mode);
}

void ClientRuntime::setIntegratorMode(const std::string &mode)
{
    _bridge.setIntegratorMode(mode);
}

void ClientRuntime::setPerformanceProfile(const std::string &profile)
{
    _bridge.setPerformanceProfile(profile);
}

void ClientRuntime::setOctreeParameters(float theta, float softening)
{
    _bridge.setOctreeParameters(theta, softening);
}

void ClientRuntime::setSphEnabled(bool enabled)
{
    _bridge.setSphEnabled(enabled);
}

void ClientRuntime::setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity)
{
    _bridge.setSphParameters(smoothingLength, restDensity, gasConstant, viscosity);
}

void ClientRuntime::setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps)
{
    _bridge.setSubstepPolicy(targetDt, maxSubsteps);
}

void ClientRuntime::setSnapshotPublishPeriodMs(std::uint32_t periodMs)
{
    _bridge.setSnapshotPublishPeriodMs(periodMs);
}

void ClientRuntime::setInitialStateConfig(const InitialStateConfig &config)
{
    _bridge.setInitialStateConfig(config);
    invalidateCachedSnapshot();
}

void ClientRuntime::setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit)
{
    _bridge.setEnergyMeasurementConfig(everySteps, sampleLimit);
}

void ClientRuntime::setExportDefaults(const std::string &directory, const std::string &format)
{
    _bridge.setExportDefaults(directory, format);
}

void ClientRuntime::setInitialStateFile(const std::string &path, const std::string &format)
{
    _bridge.setInitialStateFile(path, format);
    invalidateCachedSnapshot();
}

void ClientRuntime::requestExportSnapshot(const std::string &outputPath, const std::string &format)
{
    _bridge.requestExportSnapshot(outputPath, format);
}

void ClientRuntime::requestShutdown()
{
    _bridge.requestShutdown();
}

void ClientRuntime::setRemoteSnapshotCap(std::uint32_t maxPoints)
{
    _bridge.setRemoteSnapshotCap(maxPoints);
}

void ClientRuntime::requestReconnect()
{
    _bridge.requestReconnect();
    invalidateCachedSnapshot();
}

void ClientRuntime::configureRemoteConnector(
    const std::string &host,
    std::uint16_t port,
    bool autoStart,
    const std::string &serverExecutable)
{
    _bridge.configureRemoteConnector(host, port, autoStart, serverExecutable);
}

bool ClientRuntime::isRemoteMode() const
{
    return _bridge.isRemoteMode();
}

SimulationStats ClientRuntime::getCachedStats() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return _latestStats;
}

SimulationStats ClientRuntime::getStats() const
{
    return getCachedStats();
}

std::optional<ConsumedSnapshot> ClientRuntime::consumeLatestSnapshot()
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    if (!_hasNewSnapshot) {
        return std::nullopt;
    }
    ConsumedSnapshot snapshot{};
    snapshot.sourceSize = _latestSnapshotSize;
    snapshot.particles = std::move(_latestSnapshot);
    _hasNewSnapshot = false;
    return snapshot;
}

bool ClientRuntime::tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot)
{
    std::optional<ConsumedSnapshot> snapshot = consumeLatestSnapshot();
    if (!snapshot.has_value()) {
        return false;
    }
    outSnapshot = std::move(snapshot->particles);
    return true;
}

std::string ClientRuntime::linkStateLabel() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return _latestLinkLabel;
}

std::string ClientRuntime::serverOwnerLabel() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return _latestOwnerLabel;
}

std::uint32_t ClientRuntime::statsAgeMs() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return ageMsSince(_lastStatsAt, _hasStats);
}

std::uint32_t ClientRuntime::snapshotAgeMs() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return ageMsSince(_lastSnapshotAt, _hasSnapshotEver);
}

void ClientRuntime::invalidateCachedSnapshot()
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    _latestSnapshot.clear();
    _latestSnapshotSize = 0u;
    _hasNewSnapshot = false;
    _hasSnapshotEver = false;
}

void ClientRuntime::pollLoop()
{
    auto nextSnapshotPoll = Clock::now();
    auto nextStatusPoll = Clock::now();
    while (_pollRunning.load()) {
        const auto now = Clock::now();
        const bool pollSnapshot = now >= nextSnapshotPoll;
        const bool pollStats = now >= nextStatusPoll;
        if (pollSnapshot || pollStats) {
            pollOnce(pollSnapshot, pollStats);
            const auto afterPoll = Clock::now();
            if (pollSnapshot) {
                nextSnapshotPoll = afterPoll + kSnapshotPollInterval;
            }
            if (pollStats) {
                nextStatusPoll = afterPoll + kStatusPollInterval;
            }
            continue;
        }
        std::this_thread::sleep_for(kIdleSleepInterval);
    }
}

void ClientRuntime::pollOnce(bool pollSnapshot, bool pollStats)
{
    std::vector<RenderParticle> snapshot;
    bool gotSnapshot = false;
    if (pollSnapshot) {
        gotSnapshot = _bridge.tryConsumeSnapshot(snapshot);
    }

    SimulationStats stats{};
    if (pollStats) {
        stats = _bridge.getStats();
    }

    const std::string linkLabel(_bridge.linkStateLabel());
    const std::string ownerLabel(_bridge.serverOwnerLabel());
    const auto now = Clock::now();
    std::lock_guard<std::mutex> lock(_dataMutex);
    if (pollStats) {
        _latestStats = stats;
        _lastStatsAt = now;
        _hasStats = true;
    }
    _latestLinkLabel = linkLabel;
    _latestOwnerLabel = ownerLabel;
    if (gotSnapshot) {
        _latestSnapshotSize = snapshot.size();
        _latestSnapshot = std::move(snapshot);
        _hasNewSnapshot = true;
        _lastSnapshotAt = now;
        _hasSnapshotEver = true;
    }
}

std::uint32_t ClientRuntime::ageMsSince(Clock::time_point at, bool valid)
{
    if (!valid) {
        return std::numeric_limits<std::uint32_t>::max();
    }
    const auto now = Clock::now();
    if (at > now) {
        return 0u;
    }
    const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - at);
    const auto clamped = std::clamp<std::uint64_t>(
        static_cast<std::uint64_t>(delta.count()),
        0u,
        static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()));
    return static_cast<std::uint32_t>(clamped);
}

} // namespace grav_client
