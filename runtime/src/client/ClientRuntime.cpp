#include "client/ClientRuntime.hpp"
#include "config/SimulationConfig.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <limits>
#include <string>
#include <utility>
namespace grav_client {
constexpr auto kIdleSleepInterval = std::chrono::milliseconds(2);
constexpr auto kSnapshotPollInterval = std::chrono::milliseconds(16);
constexpr auto kStatusPollInterval = std::chrono::milliseconds(120);
constexpr std::uint32_t kSnapshotQueueCapacityMin = 1u;
constexpr std::uint32_t kSnapshotQueueCapacityMax = 64u;
constexpr std::uint32_t kSnapshotQueueCapacityDefault = 4u;
static std::string normalizeSnapshotDropPolicyValue(std::string value)
{
    for (char& c : value)
        if (c == '_')
            c = '-';
        else {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    return value;
}
ClientRuntime::ClientRuntime(const std::string& configPath, const ClientTransportArgs& transport)
    : _bridge(configPath, transport.remoteHost, transport.remotePort, transport.remoteAutoStart,
              transport.serverExecutable, transport.remoteAuthToken,
              transport.remoteCommandTimeoutMs, transport.remoteStatusTimeoutMs,
              transport.remoteSnapshotTimeoutMs),
      _pollThread(),
      _pollRunning(false),
      _dataMutex(),
      _latestStats(),
      _snapshotRing(kSnapshotQueueCapacityDefault),
      _snapshotReadIndex(0u),
      _snapshotWriteIndex(0u),
      _snapshotCount(0u),
      _droppedSnapshots(0u),
      _lastSnapshotLatencyMs(std::numeric_limits<std::uint32_t>::max()),
      _snapshotDropPolicy(SnapshotDropPolicyMode::LatestOnly),
      _latestLinkLabel("reconnecting"),
      _latestOwnerLabel("external"),
      _lastStatsAt(),
      _lastSnapshotAt(),
      _hasStats(false),
      _hasSnapshotEver(false),
      _hasDeliveredSnapshot(false)
{
    const SimulationConfig config = SimulationConfig::loadOrCreate(configPath);
    const std::uint32_t queueCapacity = std::clamp(
        config.clientSnapshotQueueCapacity, kSnapshotQueueCapacityMin, kSnapshotQueueCapacityMax);
    _snapshotRing.assign(queueCapacity, SnapshotBufferEntry{});
    _snapshotDropPolicy =
        normalizeSnapshotDropPolicyValue(config.clientSnapshotDropPolicy) == "paced"
            ? SnapshotDropPolicyMode::Paced
            : SnapshotDropPolicyMode::LatestOnly;
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
void ClientRuntime::setSolverMode(const std::string& mode)
{
    _bridge.setSolverMode(mode);
}
void ClientRuntime::setIntegratorMode(const std::string& mode)
{
    _bridge.setIntegratorMode(mode);
}
void ClientRuntime::setPerformanceProfile(const std::string& profile)
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
void ClientRuntime::setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                                     float viscosity)
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
void ClientRuntime::setInitialStateConfig(const InitialStateConfig& config)
{
    _bridge.setInitialStateConfig(config);
    invalidateCachedSnapshot();
}
void ClientRuntime::setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit)
{
    _bridge.setEnergyMeasurementConfig(everySteps, sampleLimit);
}
void ClientRuntime::setGpuTelemetryEnabled(bool enabled)
{
    _bridge.setGpuTelemetryEnabled(enabled);
}
void ClientRuntime::setExportDefaults(const std::string& directory, const std::string& format)
{
    _bridge.setExportDefaults(directory, format);
}
void ClientRuntime::setInitialStateFile(const std::string& path, const std::string& format)
{
    _bridge.setInitialStateFile(path, format);
    invalidateCachedSnapshot();
}
void ClientRuntime::requestExportSnapshot(const std::string& outputPath, const std::string& format)
{
    _bridge.requestExportSnapshot(outputPath, format);
}
void ClientRuntime::requestSaveCheckpoint(const std::string& outputPath)
{
    _bridge.requestSaveCheckpoint(outputPath);
}
void ClientRuntime::requestLoadCheckpoint(const std::string& inputPath)
{
    _bridge.requestLoadCheckpoint(inputPath);
    invalidateCachedSnapshot();
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
void ClientRuntime::configureRemoteConnector(const std::string& host, std::uint16_t port,
                                             bool autoStart, const std::string& serverExecutable)
{
    _bridge.configureRemoteConnector(host, port, autoStart, serverExecutable);
}
bool ClientRuntime::isRemoteMode() const
{
    return true;
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
    if (_snapshotCount == 0u)
        return std::nullopt;
    SnapshotBufferEntry& entry = _snapshotRing[_snapshotReadIndex];
    ConsumedSnapshot snapshot{};
    snapshot.sourceSize = entry.sourceSize;
    snapshot.particles = std::move(entry.particles);
    snapshot.latencyMs = ageMsSince(entry.receivedAt, true);
    entry.sourceSize = 0u;
    entry.receivedAt = Clock::time_point();
    _snapshotReadIndex = advanceSnapshotIndex(_snapshotReadIndex);
    _snapshotCount -= 1u;
    _lastSnapshotLatencyMs = snapshot.latencyMs;
    _hasDeliveredSnapshot = true;
    return snapshot;
}
bool ClientRuntime::tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot)
{
    std::optional<ConsumedSnapshot> snapshot = consumeLatestSnapshot();
    if (!snapshot.has_value()) {
        return false;
    }
    outSnapshot = std::move(snapshot->particles);
    return true;
}
SnapshotPipelineState ClientRuntime::snapshotPipelineState() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    SnapshotPipelineState state{};
    state.queueDepth = _snapshotCount;
    state.queueCapacity = _snapshotRing.size();
    state.droppedFrames = _droppedSnapshots;
    state.dropPolicy =
        _snapshotDropPolicy == SnapshotDropPolicyMode::Paced ? "paced" : "latest-only";
    if (_snapshotCount > 0u) {
        state.latencyMs = ageMsSince(_snapshotRing[_snapshotReadIndex].receivedAt, true);
    }
    else {
        state.latencyMs = _hasDeliveredSnapshot ? _lastSnapshotLatencyMs
                                                : std::numeric_limits<std::uint32_t>::max();
    }
    return state;
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
    for (SnapshotBufferEntry& entry : _snapshotRing)
        entry.particles.clear();
    entry.sourceSize = 0u;
    entry.receivedAt = Clock::time_point();
    _snapshotReadIndex = 0u;
    _snapshotWriteIndex = 0u;
    _snapshotCount = 0u;
    _droppedSnapshots = 0u;
    _lastSnapshotLatencyMs = std::numeric_limits<std::uint32_t>::max();
    _hasSnapshotEver = false;
    _hasDeliveredSnapshot = false;
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
            if (pollSnapshot)
                nextSnapshotPoll = afterPoll + kSnapshotPollInterval;
            if (pollStats)
                nextStatusPoll = afterPoll + kStatusPollInterval;
            continue;
        }
        std::this_thread::sleep_for(kIdleSleepInterval);
    }
}
void ClientRuntime::pollOnce(bool pollSnapshot, bool pollStats)
{
    std::vector<RenderParticle> snapshot;
    std::size_t snapshotSourceSize = 0u;
    bool gotSnapshot = false;
    if (pollSnapshot)
        gotSnapshot = _bridge.tryConsumeSnapshot(snapshot, &snapshotSourceSize);
    SimulationStats stats{};
    if (pollStats)
        stats = _bridge.getStats();
    const std::string linkLabel(_bridge.linkStateLabel());
    const std::string ownerLabel(_bridge.serverOwnerLabel());
    const auto now = Clock::now();
    std::lock_guard<std::mutex> lock(_dataMutex);
    if (pollStats)
        _latestStats = stats;
    _lastStatsAt = now;
    _hasStats = true;
    _latestLinkLabel = linkLabel;
    _latestOwnerLabel = ownerLabel;
    if (gotSnapshot) {
        queueSnapshot(std::move(snapshot), snapshotSourceSize, now);
    }
}
std::size_t ClientRuntime::advanceSnapshotIndex(std::size_t index) const
{
    return _snapshotRing.empty() ? 0u : (index + 1u) % _snapshotRing.size();
}
void ClientRuntime::queueSnapshot(std::vector<RenderParticle> snapshot, std::size_t sourceSize,
                                  Clock::time_point now)
{
    if (_snapshotRing.empty()) {
        return;
    }
    if (_snapshotCount == _snapshotRing.size()) {
        _droppedSnapshots += 1u;
        if (_snapshotDropPolicy == SnapshotDropPolicyMode::Paced) {
            return;
        }
        SnapshotBufferEntry& oldest = _snapshotRing[_snapshotReadIndex];
        oldest.particles.clear();
        oldest.sourceSize = 0u;
        oldest.receivedAt = Clock::time_point();
        _snapshotReadIndex = advanceSnapshotIndex(_snapshotReadIndex);
        _snapshotCount -= 1u;
    }
    SnapshotBufferEntry& entry = _snapshotRing[_snapshotWriteIndex];
    entry.sourceSize = sourceSize == 0u ? snapshot.size() : sourceSize;
    entry.receivedAt = now;
    entry.particles = std::move(snapshot);
    _snapshotWriteIndex = advanceSnapshotIndex(_snapshotWriteIndex);
    _snapshotCount += 1u;
    _lastSnapshotAt = now;
    _hasSnapshotEver = true;
}
std::uint32_t ClientRuntime::ageMsSince(Clock::time_point at, bool valid)
{
    if (!valid)
        return std::numeric_limits<std::uint32_t>::max();
    const auto now = Clock::now();
    if (at > now)
        return 0u;
    const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - at);
    const auto clamped = std::clamp<std::uint64_t>(
        static_cast<std::uint64_t>(delta.count()), 0u,
        static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()));
    return static_cast<std::uint32_t>(clamped);
}
} // namespace grav_client
