/*
 * @file runtime/include/client/IClientRuntime.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_ICLIENTRUNTIME_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_ICLIENTRUNTIME_HPP_
/*
 * Module: client
 * Responsibility: Expose the deterministic runtime control and telemetry
 * contract used by UI
 * frontends.
 */
#include "types/SimulationTypes.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace grav_client {
struct ConsumedSnapshot final {
    std::vector<RenderParticle> particles;
    std::size_t sourceSize = 0u;
    std::uint32_t latencyMs = 0u;
};

struct SnapshotPipelineState final {
    std::size_t queueDepth = 0u;
    std::size_t queueCapacity = 0u;
    std::uint64_t droppedFrames = 0u;
    std::uint32_t latencyMs = 0u;
    std::string dropPolicy = "latest-only";
};

class IClientRuntime {
public:
    virtual ~IClientRuntime() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void setPaused(bool paused) = 0;
    virtual void togglePaused() = 0;
    virtual void stepOnce() = 0;
    virtual void setParticleCount(std::uint32_t particleCount) = 0;
    virtual void setDt(float dt) = 0;
    virtual void scaleDt(float factor) = 0;
    virtual void requestReset() = 0;
    virtual void requestRecover() = 0;
    virtual void setSolverMode(const std::string& mode) = 0;
    virtual void setIntegratorMode(const std::string& mode) = 0;
    virtual void setPerformanceProfile(const std::string& profile) = 0;
    virtual void setOctreeParameters(float theta, float softening) = 0;
    virtual void setSphEnabled(bool enabled) = 0;
    virtual void setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                                  float viscosity) = 0;
    virtual void setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps) = 0;
    virtual void setSnapshotPublishPeriodMs(std::uint32_t periodMs) = 0;
    virtual void setInitialStateConfig(const InitialStateConfig& config) = 0;
    virtual void setEnergyMeasurementConfig(std::uint32_t everySteps,
                                            std::uint32_t sampleLimit) = 0;
    virtual void setGpuTelemetryEnabled(bool enabled) = 0;
    virtual void setExportDefaults(const std::string& directory, const std::string& format) = 0;
    virtual void setInitialStateFile(const std::string& path, const std::string& format) = 0;
    virtual void requestExportSnapshot(const std::string& outputPath,
                                       const std::string& format) = 0;
    virtual void requestSaveCheckpoint(const std::string& outputPath) = 0;
    virtual void requestLoadCheckpoint(const std::string& inputPath) = 0;
    virtual void requestShutdown() = 0;
    virtual void setRemoteSnapshotCap(std::uint32_t maxPoints) = 0;
    virtual void requestReconnect() = 0;
    virtual void configureRemoteConnector(const std::string& host, std::uint16_t port,
                                          bool autoStart, const std::string& serverExecutable) = 0;
    virtual bool isRemoteMode() const = 0;
    virtual SimulationStats getCachedStats() const = 0;
    virtual SimulationStats getStats() const = 0;
    virtual std::optional<ConsumedSnapshot> consumeLatestSnapshot() = 0;
    virtual bool tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot) = 0;
    virtual SnapshotPipelineState snapshotPipelineState() const = 0;
    virtual std::string linkStateLabel() const = 0;
    virtual std::string serverOwnerLabel() const = 0;
    virtual std::uint32_t statsAgeMs() const = 0;
    virtual std::uint32_t snapshotAgeMs() const = 0;
};
} // namespace grav_client
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_ICLIENTRUNTIME_HPP_
