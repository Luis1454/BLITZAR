#include "runtime/src/ffi/BlitzarCoreInternal.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <thread>
static constexpr std::chrono::milliseconds kPollInterval(5);
static bool hasMatchingConfig(const SimulationStats& stats, const blitzar_core_config_t& config)
{
    return stats.particleCount == std::max<std::uint32_t>(2u, config.particle_count) &&
           stats.dt == config.dt && stats.solverName == config.solver_name &&
           stats.integratorName == config.integrator_name &&
           stats.performanceProfile == config.performance_profile &&
           stats.maxSubsteps == std::max<std::uint32_t>(1u, config.max_substeps) &&
           stats.snapshotPublishPeriodMs == config.snapshot_publish_period_ms;
}
template <typename TPredicate> static bool pollUntil(std::uint32_t timeoutMs, TPredicate predicate)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() <= deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(kPollInterval);
    }
    return false;
}
namespace grav_ffi {
blitzar_core_result_t BlitzarCore::runSteps(std::uint32_t steps, std::uint32_t timeoutMs)
{
    clearError();
    if (steps == 0u) {
        return BLITZAR_CORE_OK;
    }
    _server.setPaused(true);
    const std::uint64_t startSteps = _server.getStats().steps;
    for (std::uint32_t index = 0; index < steps; ++index) {
        _server.stepOnce();
    }
    return waitForStepTarget(startSteps + steps, timeoutMs);
}
blitzar_core_result_t BlitzarCore::loadState(const char* path, const char* format,
                                             std::uint32_t timeoutMs)
{
    clearError();
    if (path == nullptr || path[0] == '\0') {
        setError("load path is required");
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    _initialStateConfig.mode = "file";
    _server.setInitialStateConfig(_initialStateConfig);
    _server.setInitialStateFile(path, format == nullptr ? "" : format);
    _server.setPaused(true);
    return waitForSnapshot(timeoutMs);
}
blitzar_core_result_t BlitzarCore::exportState(const char* path, const char* format,
                                               std::uint32_t timeoutMs)
{
    clearError();
    if (path == nullptr || path[0] == '\0') {
        setError("export path is required");
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    _server.requestExportSnapshot(path, format == nullptr ? "" : format);
    return waitForFile(path, timeoutMs);
}
std::size_t BlitzarCore::copyLastError(char* buffer, std::size_t capacity) const
{
    std::lock_guard<std::mutex> lock(_errorMutex);
    if (buffer == nullptr || capacity == 0u) {
        return _lastError.size();
    }
    const std::size_t limit = std::min<std::size_t>(capacity - 1u, _lastError.size());
    std::memcpy(buffer, _lastError.data(), limit);
    buffer[limit] = '\0';
    return _lastError.size();
}
blitzar_core_result_t BlitzarCore::waitForSnapshot(std::uint32_t timeoutMs) const
{
    std::vector<RenderParticle> snapshot;
    if (pollUntil(timeoutMs, [this, &snapshot]() {
            return _server.copyLatestSnapshot(snapshot, 0u);
        })) {
        return BLITZAR_CORE_OK;
    }
    setError("timed out waiting for snapshot");
    return BLITZAR_CORE_TIMEOUT;
}
blitzar_core_result_t BlitzarCore::waitForAppliedConfig(const blitzar_core_config_t& config,
                                                        std::uint32_t timeoutMs) const
{
    std::vector<RenderParticle> snapshot;
    if (pollUntil(timeoutMs, [this, &config, &snapshot]() {
            return hasMatchingConfig(_server.getStats(), config) &&
                   _server.copyLatestSnapshot(snapshot, 0u);
        })) {
        return BLITZAR_CORE_OK;
    }
    setError("timed out waiting for config application");
    return BLITZAR_CORE_TIMEOUT;
}
blitzar_core_result_t BlitzarCore::waitForStepTarget(std::uint64_t expectedSteps,
                                                     std::uint32_t timeoutMs) const
{
    if (pollUntil(timeoutMs, [this, expectedSteps]() {
            const SimulationStats stats = _server.getStats();
            return !stats.faulted && stats.steps >= expectedSteps;
        })) {
        return BLITZAR_CORE_OK;
    }
    const SimulationStats stats = _server.getStats();
    setError(stats.faulted ? stats.faultReason : "timed out waiting for requested steps");
    return stats.faulted ? BLITZAR_CORE_INTERNAL_ERROR : BLITZAR_CORE_TIMEOUT;
}
blitzar_core_result_t BlitzarCore::waitForFile(const char* path, std::uint32_t timeoutMs) const
{
    const std::filesystem::path fsPath(path);
    if (pollUntil(timeoutMs, [fsPath]() {
            return std::filesystem::exists(fsPath) &&
                   std::filesystem::file_size(fsPath) > 0u;
        })) {
        return BLITZAR_CORE_OK;
    }
    setError("timed out waiting for exported state file");
    return BLITZAR_CORE_TIMEOUT;
}
void BlitzarCore::setError(const std::string& message) const
{
    std::lock_guard<std::mutex> lock(_errorMutex);
    _lastError = message;
}
void BlitzarCore::clearError() const
{
    setError("");
}
} // namespace grav_ffi
