#include "sim/FrontendRuntime.hpp"
#include "sim/LocalBackendFactory.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>

namespace sim {

namespace {
constexpr auto kIdleSleepInterval = std::chrono::milliseconds(2);
constexpr auto kSnapshotPollInterval = std::chrono::milliseconds(16);
constexpr auto kStatusPollInterval = std::chrono::milliseconds(120);
}

FrontendRuntime::FrontendRuntime(const std::string &configPath, const FrontendTransportArgs &transport)
    : _bridge(
          configPath,
          transport.remoteMode,
          transport.remoteHost,
          transport.remotePort,
          transport.remoteAutoStart,
          transport.backendExecutable,
          transport.remoteAuthToken,
          transport.remoteCommandTimeoutMs,
          transport.remoteStatusTimeoutMs,
          transport.remoteSnapshotTimeoutMs,
          createLocalBackend),
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

FrontendRuntime::~FrontendRuntime()
{
    stop();
}

bool FrontendRuntime::start()
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

void FrontendRuntime::stop()
{
    _pollRunning.store(false);
    if (_pollThread.joinable()) {
        _pollThread.join();
    }
    _bridge.stop();
}

void FrontendRuntime::setPaused(bool paused)
{
    _bridge.setPaused(paused);
}

void FrontendRuntime::togglePaused()
{
    _bridge.togglePaused();
}

void FrontendRuntime::stepOnce()
{
    _bridge.stepOnce();
}

void FrontendRuntime::setParticleCount(std::uint32_t particleCount)
{
    _bridge.setParticleCount(particleCount);
}

void FrontendRuntime::setDt(float dt)
{
    _bridge.setDt(dt);
}

void FrontendRuntime::scaleDt(float factor)
{
    _bridge.scaleDt(factor);
}

void FrontendRuntime::requestReset()
{
    _bridge.requestReset();
}

void FrontendRuntime::requestRecover()
{
    _bridge.requestRecover();
}

void FrontendRuntime::setSolverMode(const std::string &mode)
{
    _bridge.setSolverMode(mode);
}

void FrontendRuntime::setIntegratorMode(const std::string &mode)
{
    _bridge.setIntegratorMode(mode);
}

void FrontendRuntime::setOctreeParameters(float theta, float softening)
{
    _bridge.setOctreeParameters(theta, softening);
}

void FrontendRuntime::setSphEnabled(bool enabled)
{
    _bridge.setSphEnabled(enabled);
}

void FrontendRuntime::setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity)
{
    _bridge.setSphParameters(smoothingLength, restDensity, gasConstant, viscosity);
}

void FrontendRuntime::setInitialStateConfig(const InitialStateConfig &config)
{
    _bridge.setInitialStateConfig(config);
}

void FrontendRuntime::setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit)
{
    _bridge.setEnergyMeasurementConfig(everySteps, sampleLimit);
}

void FrontendRuntime::setExportDefaults(const std::string &directory, const std::string &format)
{
    _bridge.setExportDefaults(directory, format);
}

void FrontendRuntime::setInitialStateFile(const std::string &path, const std::string &format)
{
    _bridge.setInitialStateFile(path, format);
}

void FrontendRuntime::requestExportSnapshot(const std::string &outputPath, const std::string &format)
{
    _bridge.requestExportSnapshot(outputPath, format);
}

void FrontendRuntime::requestShutdown()
{
    _bridge.requestShutdown();
}

void FrontendRuntime::setRemoteSnapshotCap(std::uint32_t maxPoints)
{
    _bridge.setRemoteSnapshotCap(maxPoints);
}

void FrontendRuntime::requestReconnect()
{
    _bridge.requestReconnect();
}

void FrontendRuntime::configureRemoteConnector(
    const std::string &host,
    std::uint16_t port,
    bool autoStart,
    const std::string &backendExecutable)
{
    _bridge.configureRemoteConnector(host, port, autoStart, backendExecutable);
}

bool FrontendRuntime::isRemoteMode() const
{
    return _bridge.isRemoteMode();
}

SimulationStats FrontendRuntime::getCachedStats() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return _latestStats;
}

SimulationStats FrontendRuntime::getStats() const
{
    return getCachedStats();
}

bool FrontendRuntime::consumeLatestSnapshot(std::vector<RenderParticle> &outSnapshot, std::size_t *snapshotSize)
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    if (!_hasNewSnapshot) {
        return false;
    }
    outSnapshot = std::move(_latestSnapshot);
    _hasNewSnapshot = false;
    if (snapshotSize != nullptr) {
        *snapshotSize = _latestSnapshotSize;
    }
    return true;
}

bool FrontendRuntime::tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot)
{
    return consumeLatestSnapshot(outSnapshot, nullptr);
}

std::string FrontendRuntime::linkStateLabel() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return _latestLinkLabel;
}

std::string FrontendRuntime::backendOwnerLabel() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return _latestOwnerLabel;
}

std::uint32_t FrontendRuntime::statsAgeMs() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return ageMsSince(_lastStatsAt, _hasStats);
}

std::uint32_t FrontendRuntime::snapshotAgeMs() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return ageMsSince(_lastSnapshotAt, _hasSnapshotEver);
}

void FrontendRuntime::pollLoop()
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

void FrontendRuntime::pollOnce(bool pollSnapshot, bool pollStats)
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
    const std::string ownerLabel(_bridge.backendOwnerLabel());
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

std::uint32_t FrontendRuntime::ageMsSince(Clock::time_point at, bool valid)
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

} // namespace sim
