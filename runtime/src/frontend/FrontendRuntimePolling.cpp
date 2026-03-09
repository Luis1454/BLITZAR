#include "frontend/FrontendRuntimePolling.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>

namespace grav_frontend {

constexpr auto kIdleSleepInterval = std::chrono::milliseconds(2);
constexpr auto kSnapshotPollInterval = std::chrono::milliseconds(16);
constexpr auto kStatusPollInterval = std::chrono::milliseconds(120);

SimulationStats FrontendRuntime::getCachedStats() const
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return _latestStats;
}

SimulationStats FrontendRuntime::getStats() const
{
    return getCachedStats();
}

std::optional<ConsumedSnapshot> FrontendRuntime::consumeLatestSnapshot()
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

bool FrontendRuntime::tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot)
{
    std::optional<ConsumedSnapshot> snapshot = consumeLatestSnapshot();
    if (!snapshot.has_value()) {
        return false;
    }
    outSnapshot = std::move(snapshot->particles);
    return true;
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

} // namespace grav_frontend
