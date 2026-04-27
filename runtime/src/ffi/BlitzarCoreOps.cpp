// File: runtime/src/ffi/BlitzarCoreOps.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "runtime/src/ffi/BlitzarCoreInternal.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <thread>
/// Description: Executes the kPollInterval operation.
static constexpr std::chrono::milliseconds kPollInterval(5);
/// Description: Executes the hasMatchingConfig operation.
static bool hasMatchingConfig(const SimulationStats& stats, const blitzar_core_config_t& config)
{
    return stats.particleCount == std::max<std::uint32_t>(2u, config.particle_count) &&
           stats.dt == config.dt && stats.solverName == config.solver_name &&
           stats.integratorName == config.integrator_name &&
           stats.performanceProfile == config.performance_profile &&
           stats.maxSubsteps == std::max<std::uint32_t>(1u, config.max_substeps) &&
           stats.snapshotPublishPeriodMs == config.snapshot_publish_period_ms;
}
/// Description: Executes the pollUntil operation.
template <typename TPredicate> static bool pollUntil(std::uint32_t timeoutMs, TPredicate predicate)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() <= deadline) {
        if (predicate()) {
            return true;
        }
        /// Description: Executes the sleep_for operation.
        std::this_thread::sleep_for(kPollInterval);
    }
    return false;
}
namespace grav_ffi {
/// Description: Executes the runSteps operation.
blitzar_core_result_t BlitzarCore::runSteps(std::uint32_t steps, std::uint32_t timeoutMs)
{
    /// Description: Executes the clearError operation.
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
    /// Description: Executes the clearError operation.
    clearError();
    if (path == nullptr || path[0] == '\0') {
        /// Description: Executes the setError operation.
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
    /// Description: Executes the clearError operation.
    clearError();
    if (path == nullptr || path[0] == '\0') {
        /// Description: Executes the setError operation.
        setError("export path is required");
        return BLITZAR_CORE_INVALID_ARGUMENT;
    }
    _server.requestExportSnapshot(path, format == nullptr ? "" : format);
    return waitForFile(path, timeoutMs);
}
/// Description: Executes the copyLastError operation.
std::size_t BlitzarCore::copyLastError(char* buffer, std::size_t capacity) const
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::mutex> lock(_errorMutex);
    if (buffer == nullptr || capacity == 0u) {
        return _lastError.size();
    }
    const std::size_t limit = std::min<std::size_t>(capacity - 1u, _lastError.size());
    /// Description: Executes the memcpy operation.
    std::memcpy(buffer, _lastError.data(), limit);
    buffer[limit] = '\0';
    return _lastError.size();
}
/// Description: Executes the waitForSnapshot operation.
blitzar_core_result_t BlitzarCore::waitForSnapshot(std::uint32_t timeoutMs) const
{
    std::vector<RenderParticle> snapshot;
    if (pollUntil(timeoutMs, [this, &snapshot]() {
            return _server.copyLatestSnapshot(snapshot, 0u);
        })) {
        return BLITZAR_CORE_OK;
    }
    /// Description: Executes the setError operation.
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
    /// Description: Executes the setError operation.
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
    /// Description: Executes the setError operation.
    setError(stats.faulted ? stats.faultReason : "timed out waiting for requested steps");
    return stats.faulted ? BLITZAR_CORE_INTERNAL_ERROR : BLITZAR_CORE_TIMEOUT;
}
/// Description: Executes the waitForFile operation.
blitzar_core_result_t BlitzarCore::waitForFile(const char* path, std::uint32_t timeoutMs) const
{
    /// Description: Executes the fsPath operation.
    const std::filesystem::path fsPath(path);
    if (pollUntil(timeoutMs, [fsPath]() {
            return std::filesystem::exists(fsPath) &&
                   std::filesystem::file_size(fsPath) > 0u;
        })) {
        return BLITZAR_CORE_OK;
    }
    /// Description: Executes the setError operation.
    setError("timed out waiting for exported state file");
    return BLITZAR_CORE_TIMEOUT;
}
/// Description: Executes the setError operation.
void BlitzarCore::setError(const std::string& message) const
{
    /// Description: Executes the lock operation.
    std::lock_guard<std::mutex> lock(_errorMutex);
    _lastError = message;
}
/// Description: Executes the clearError operation.
void BlitzarCore::clearError() const
{
    /// Description: Executes the setError operation.
    setError("");
}
} // namespace grav_ffi
