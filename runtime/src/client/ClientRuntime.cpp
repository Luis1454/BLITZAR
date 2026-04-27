// File: runtime/src/client/ClientRuntime.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

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

/// Description: Executes the normalizeSnapshotDropPolicyValue operation.
static std::string normalizeSnapshotDropPolicyValue(std::string value)
{
    for (char& c : value) {
        if (c == '_') {
            c = '-';
        }
        else {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return value;
}

/// Description: Executes the ClientRuntime operation.
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

/// Description: Releases resources owned by ClientRuntime.
ClientRuntime::~ClientRuntime()
{
    stop();
}

/// Description: Executes the start operation.
bool ClientRuntime::start()
{
    stop();
    if (!_bridge.start()) {
        return false;
    }
    pollOnce(true, true);
    _pollRunning.store(true);
    _pollThread = std::thread([this]() {
        pollLoop();
    });
    return true;
}

/// Description: Executes the stop operation.
void ClientRuntime::stop()
{
    _pollRunning.store(false);
    if (_pollThread.joinable()) {
        _pollThread.join();
    }
    _bridge.stop();
}

/// Description: Executes the setPaused operation.
void ClientRuntime::setPaused(bool paused)
{
    _bridge.setPaused(paused);
}

/// Description: Executes the togglePaused operation.
void ClientRuntime::togglePaused()
{
    _bridge.togglePaused();
}

/// Description: Executes the stepOnce operation.
void ClientRuntime::stepOnce()
{
    _bridge.stepOnce();
}

/// Description: Executes the setParticleCount operation.
void ClientRuntime::setParticleCount(std::uint32_t particleCount)
{
    _bridge.setParticleCount(particleCount);
}

/// Description: Executes the setDt operation.
void ClientRuntime::setDt(float dt)
{
    _bridge.setDt(dt);
}

/// Description: Executes the scaleDt operation.
void ClientRuntime::scaleDt(float factor)
{
    _bridge.scaleDt(factor);
}

/// Description: Executes the requestReset operation.
void ClientRuntime::requestReset()
{
    _bridge.requestReset();
    invalidateCachedSnapshot();
}

/// Description: Executes the requestRecover operation.
void ClientRuntime::requestRecover()
{
    _bridge.requestRecover();
    invalidateCachedSnapshot();
}

/// Description: Executes the setSolverMode operation.
void ClientRuntime::setSolverMode(const std::string& mode)
{
    _bridge.setSolverMode(mode);
}

/// Description: Executes the setIntegratorMode operation.
void ClientRuntime::setIntegratorMode(const std::string& mode)
{
    _bridge.setIntegratorMode(mode);
}

/// Description: Executes the setPerformanceProfile operation.
void ClientRuntime::setPerformanceProfile(const std::string& profile)
{
    _bridge.setPerformanceProfile(profile);
}

/// Description: Executes the setOctreeParameters operation.
void ClientRuntime::setOctreeParameters(float theta, float softening)
{
    _bridge.setOctreeParameters(theta, softening);
}

/// Description: Executes the setSphEnabled operation.
void ClientRuntime::setSphEnabled(bool enabled)
{
    _bridge.setSphEnabled(enabled);
}

/// Description: Describes the set sph parameters operation contract.
void ClientRuntime::setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                                     float viscosity)
{
    _bridge.setSphParameters(smoothingLength, restDensity, gasConstant, viscosity);
}

/// Description: Executes the setSubstepPolicy operation.
void ClientRuntime::setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps)
{
    _bridge.setSubstepPolicy(targetDt, maxSubsteps);
}

/// Description: Executes the setSnapshotPublishPeriodMs operation.
void ClientRuntime::setSnapshotPublishPeriodMs(std::uint32_t periodMs)
{
    _bridge.setSnapshotPublishPeriodMs(periodMs);
}

/// Description: Executes the setInitialStateConfig operation.
void ClientRuntime::setInitialStateConfig(const InitialStateConfig& config)
{
    _bridge.setInitialStateConfig(config);
    invalidateCachedSnapshot();
}

/// Description: Executes the setEnergyMeasurementConfig operation.
void ClientRuntime::setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit)
{
    _bridge.setEnergyMeasurementConfig(everySteps, sampleLimit);
}

/// Description: Executes the setGpuTelemetryEnabled operation.
void ClientRuntime::setGpuTelemetryEnabled(bool enabled)
{
    _bridge.setGpuTelemetryEnabled(enabled);
}

/// Description: Executes the setExportDefaults operation.
void ClientRuntime::setExportDefaults(const std::string& directory, const std::string& format)
{
    _bridge.setExportDefaults(directory, format);
}

/// Description: Executes the setInitialStateFile operation.
void ClientRuntime::setInitialStateFile(const std::string& path, const std::string& format)
{
    _bridge.setInitialStateFile(path, format);
    invalidateCachedSnapshot();
}

/// Description: Executes the requestExportSnapshot operation.
void ClientRuntime::requestExportSnapshot(const std::string& outputPath, const std::string& format)
{
    _bridge.requestExportSnapshot(outputPath, format);
}

/// Description: Executes the requestSaveCheckpoint operation.
void ClientRuntime::requestSaveCheckpoint(const std::string& outputPath)
{
    _bridge.requestSaveCheckpoint(outputPath);
}

/// Description: Executes the requestLoadCheckpoint operation.
void ClientRuntime::requestLoadCheckpoint(const std::string& inputPath)
{
    _bridge.requestLoadCheckpoint(inputPath);
    invalidateCachedSnapshot();
}

/// Description: Executes the requestShutdown operation.
void ClientRuntime::requestShutdown()
{
    _bridge.requestShutdown();
}

/// Description: Executes the setRemoteSnapshotCap operation.
void ClientRuntime::setRemoteSnapshotCap(std::uint32_t maxPoints)
{
    _bridge.setRemoteSnapshotCap(maxPoints);
}

/// Description: Executes the requestReconnect operation.
void ClientRuntime::requestReconnect()
{
    _bridge.requestReconnect();
    invalidateCachedSnapshot();
}

/// Description: Describes the configure remote connector operation contract.
void ClientRuntime::configureRemoteConnector(const std::string& host, std::uint16_t port,
                                             bool autoStart, const std::string& serverExecutable)
{
    _bridge.configureRemoteConnector(host, port, autoStart, serverExecutable);
}

/// Description: Executes the isRemoteMode operation.
bool ClientRuntime::isRemoteMode() const
{
    return true;
}

/// Description: Executes the getCachedStats operation.
SimulationStats ClientRuntime::getCachedStats() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return _latestStats;
}

/// Description: Executes the getStats operation.
SimulationStats ClientRuntime::getStats() const
{
    return getCachedStats();
}

/// Description: Executes the consumeLatestSnapshot operation.
std::optional<ConsumedSnapshot> ClientRuntime::consumeLatestSnapshot()
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    if (_snapshotCount == 0u) {
        return std::nullopt;
    }
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

/// Description: Executes the tryConsumeSnapshot operation.
bool ClientRuntime::tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot)
{
    std::optional<ConsumedSnapshot> snapshot = consumeLatestSnapshot();
    if (!snapshot.has_value()) {
        return false;
    }
    outSnapshot = std::move(snapshot->particles);
    return true;
}

/// Description: Executes the snapshotPipelineState operation.
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

/// Description: Executes the linkStateLabel operation.
std::string ClientRuntime::linkStateLabel() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return _latestLinkLabel;
}

/// Description: Executes the serverOwnerLabel operation.
std::string ClientRuntime::serverOwnerLabel() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return _latestOwnerLabel;
}

/// Description: Executes the statsAgeMs operation.
std::uint32_t ClientRuntime::statsAgeMs() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return ageMsSince(_lastStatsAt, _hasStats);
}

/// Description: Executes the snapshotAgeMs operation.
std::uint32_t ClientRuntime::snapshotAgeMs() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return ageMsSince(_lastSnapshotAt, _hasSnapshotEver);
}

/// Description: Executes the invalidateCachedSnapshot operation.
void ClientRuntime::invalidateCachedSnapshot()
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    for (SnapshotBufferEntry& entry : _snapshotRing) {
        entry.particles.clear();
        entry.sourceSize = 0u;
        entry.receivedAt = Clock::time_point();
    }
    _snapshotReadIndex = 0u;
    _snapshotWriteIndex = 0u;
    _snapshotCount = 0u;
    _droppedSnapshots = 0u;
    _lastSnapshotLatencyMs = std::numeric_limits<std::uint32_t>::max();
    _hasSnapshotEver = false;
    _hasDeliveredSnapshot = false;
}

/// Description: Executes the pollLoop operation.
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

/// Description: Executes the pollOnce operation.
void ClientRuntime::pollOnce(bool pollSnapshot, bool pollStats)
{
    std::vector<RenderParticle> snapshot;
    std::size_t snapshotSourceSize = 0u;
    bool gotSnapshot = false;
    if (pollSnapshot) {
        gotSnapshot = _bridge.tryConsumeSnapshot(snapshot, &snapshotSourceSize);
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
        queueSnapshot(std::move(snapshot), snapshotSourceSize, now);
    }
}

/// Description: Executes the advanceSnapshotIndex operation.
std::size_t ClientRuntime::advanceSnapshotIndex(std::size_t index) const
{
    return _snapshotRing.empty() ? 0u : (index + 1u) % _snapshotRing.size();
}

/// Description: Describes the queue snapshot operation contract.
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

/// Description: Executes the ageMsSince operation.
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
        static_cast<std::uint64_t>(delta.count()), 0u,
        static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()));
    return static_cast<std::uint32_t>(clamped);
}
} // namespace grav_client
