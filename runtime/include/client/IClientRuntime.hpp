#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_ICLIENTRUNTIME_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_ICLIENTRUNTIME_HPP_
/* * Module: client * Responsibility: Expose the deterministic runtime control and telemetry
 * contract used by UI * frontends. */
#include "types/SimulationTypes.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace grav_client {
/// Carries the latest rendered snapshot consumed by a frontend together with transport metadata.
struct ConsumedSnapshot final {
    std::vector<RenderParticle> particles;
    std::size_t sourceSize = 0u;
    std::uint32_t latencyMs = 0u;
};
/// Summarizes the state of the client-side snapshot queue.
struct SnapshotPipelineState final {
    std::size_t queueDepth = 0u;
    std::size_t queueCapacity = 0u;
    std::uint64_t droppedFrames = 0u;
    std::uint32_t latencyMs = 0u;
    std::string dropPolicy = "latest-only";
};
/// Defines the control and telemetry surface consumed by interactive and scripted client frontends.
class IClientRuntime {
public:
    /// Releases the runtime implementation and any owned transport resources.
    virtual ~IClientRuntime() = default;
    /// Starts the runtime loop and any required transport connections.
    virtual bool start() = 0;
    /// Stops the runtime loop and releases remote resources.
    virtual void stop() = 0;
    /// Forces the simulation into the requested paused state.
    virtual void setPaused(bool paused) = 0;
    /// Toggles the paused state without changing any other runtime settings.
    virtual void togglePaused() = 0;
    /// Advances the simulation by exactly one deterministic step while paused.
    virtual void stepOnce() = 0;
    /// Rebuilds the runtime with a new particle count.
    virtual void setParticleCount(std::uint32_t particleCount) = 0;
    /// Sets the primary simulation step size in seconds.
    virtual void setDt(float dt) = 0;
    /// Multiplies the current simulation step size by `factor`.
    virtual void scaleDt(float factor) = 0;
    /// Reinitializes the current scenario using the active configuration.
    virtual void requestReset() = 0;
    /// Requests recovery from a runtime fault without discarding the active configuration.
    virtual void requestRecover() = 0;
    /// Selects the gravity solver implementation by name.
    virtual void setSolverMode(const std::string& mode) = 0;
    /// Selects the numerical integrator by name.
    virtual void setIntegratorMode(const std::string& mode) = 0;
    /// Applies a named performance profile to the runtime.
    virtual void setPerformanceProfile(const std::string& profile) = 0;
    /// Updates the octree opening and softening parameters.
    virtual void setOctreeParameters(float theta, float softening) = 0;
    /// Enables or disables SPH processing.
    virtual void setSphEnabled(bool enabled) = 0;
    /// Updates the SPH model parameters in SI units.
    virtual void setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                                  float viscosity) = 0;
    /// Configures substep sizing used by deterministic stepping policies.
    virtual void setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps) = 0;
    /// Sets the remote snapshot publication period in milliseconds.
    virtual void setSnapshotPublishPeriodMs(std::uint32_t periodMs) = 0;
    /// Replaces the current procedural initial-state configuration.
    virtual void setInitialStateConfig(const InitialStateConfig& config) = 0;
    /// Controls how often energy samples are measured and retained.
    virtual void setEnergyMeasurementConfig(std::uint32_t everySteps,
                                            std::uint32_t sampleLimit) = 0;
    /// Enables or disables bounded GPU telemetry sampling on the server.
    virtual void setGpuTelemetryEnabled(bool enabled) = 0;
    /// Sets default output parameters used by snapshot export requests.
    virtual void setExportDefaults(const std::string& directory, const std::string& format) = 0;
    /// Selects a file-backed initial state and its decoding format.
    virtual void setInitialStateFile(const std::string& path, const std::string& format) = 0;
    /// Exports the current simulation snapshot to `outputPath` using `format`.
    virtual void requestExportSnapshot(const std::string& outputPath,
                                       const std::string& format) = 0;
    /// Saves a restartable checkpoint to `outputPath`.
    virtual void requestSaveCheckpoint(const std::string& outputPath) = 0;
    /// Loads a restartable checkpoint from `inputPath`.
    virtual void requestLoadCheckpoint(const std::string& inputPath) = 0;
    /// Requests a remote runtime shutdown.
    virtual void requestShutdown() = 0;
    /// Limits the number of particles retained in the remote snapshot path.
    virtual void setRemoteSnapshotCap(std::uint32_t maxPoints) = 0;
    /// Re-establishes the active remote connection with the current connector settings.
    virtual void requestReconnect() = 0;
    /// Updates the remote connector endpoint and auto-start settings.
    virtual void configureRemoteConnector(const std::string& host, std::uint16_t port,
                                          bool autoStart, const std::string& serverExecutable) = 0;
    /// Reports whether the runtime is currently driven by a remote server.
    virtual bool isRemoteMode() const = 0;
    /// Returns the most recently cached stats without forcing a transport round-trip.
    virtual SimulationStats getCachedStats() const = 0;
    /// Returns the freshest available simulation stats.
    virtual SimulationStats getStats() const = 0;
    /// Consumes the latest snapshot together with queue metadata.
    virtual std::optional<ConsumedSnapshot> consumeLatestSnapshot() = 0;
    /// Copies the latest snapshot into `outSnapshot` when one is available.
    virtual bool tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot) = 0;
    /// Returns queue depth, latency, and drop information for snapshot delivery.
    virtual SnapshotPipelineState snapshotPipelineState() const = 0;
    /// Returns a short user-facing link state label.
    virtual std::string linkStateLabel() const = 0;
    /// Returns a user-facing ownership label for the current server endpoint.
    virtual std::string serverOwnerLabel() const = 0;
    /// Returns the age in milliseconds of the last cached stats sample.
    virtual std::uint32_t statsAgeMs() const = 0;
    /// Returns the age in milliseconds of the last delivered snapshot.
    virtual std::uint32_t snapshotAgeMs() const = 0;
};
} // namespace grav_client
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_ICLIENTRUNTIME_HPP_
