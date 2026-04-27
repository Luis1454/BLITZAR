// File: runtime/include/client/ClientRuntime.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTRUNTIME_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTRUNTIME_HPP_
#include "client/ClientServerBridge.hpp"
#include "client/IClientRuntime.hpp"
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace grav_client {
/// Description: Defines the ClientRuntime data or behavior contract.
class ClientRuntime final : public IClientRuntime {
public:
    /// Description: Describes the client runtime operation contract.
    ClientRuntime(const std::string& configPath, const ClientTransportArgs& transport);
    /// Description: Releases resources owned by ClientRuntime.
    ~ClientRuntime() override;
    /// Description: Describes the start operation contract.
    bool start() override;
    /// Description: Describes the stop operation contract.
    void stop() override;
    /// Description: Describes the set paused operation contract.
    void setPaused(bool paused) override;
    /// Description: Describes the toggle paused operation contract.
    void togglePaused() override;
    /// Description: Describes the step once operation contract.
    void stepOnce() override;
    /// Description: Describes the set particle count operation contract.
    void setParticleCount(std::uint32_t particleCount) override;
    /// Description: Describes the set dt operation contract.
    void setDt(float dt) override;
    /// Description: Describes the scale dt operation contract.
    void scaleDt(float factor) override;
    /// Description: Describes the request reset operation contract.
    void requestReset() override;
    /// Description: Describes the request recover operation contract.
    void requestRecover() override;
    /// Description: Describes the set solver mode operation contract.
    void setSolverMode(const std::string& mode) override;
    /// Description: Describes the set integrator mode operation contract.
    void setIntegratorMode(const std::string& mode) override;
    /// Description: Describes the set performance profile operation contract.
    void setPerformanceProfile(const std::string& profile) override;
    /// Description: Describes the set octree parameters operation contract.
    void setOctreeParameters(float theta, float softening) override;
    /// Description: Describes the set sph enabled operation contract.
    void setSphEnabled(bool enabled) override;
    /// Description: Describes the set sph parameters operation contract.
    void setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                          float viscosity) override;
    /// Description: Describes the set substep policy operation contract.
    void setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps) override;
    /// Description: Describes the set snapshot publish period ms operation contract.
    void setSnapshotPublishPeriodMs(std::uint32_t periodMs) override;
    /// Description: Describes the set initial state config operation contract.
    void setInitialStateConfig(const InitialStateConfig& config) override;
    /// Description: Describes the set energy measurement config operation contract.
    void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit) override;
    /// Description: Describes the set gpu telemetry enabled operation contract.
    void setGpuTelemetryEnabled(bool enabled) override;
    /// Description: Describes the set export defaults operation contract.
    void setExportDefaults(const std::string& directory, const std::string& format) override;
    /// Description: Describes the set initial state file operation contract.
    void setInitialStateFile(const std::string& path, const std::string& format) override;
    /// Description: Describes the request export snapshot operation contract.
    void requestExportSnapshot(const std::string& outputPath, const std::string& format) override;
    /// Description: Describes the request save checkpoint operation contract.
    void requestSaveCheckpoint(const std::string& outputPath) override;
    /// Description: Describes the request load checkpoint operation contract.
    void requestLoadCheckpoint(const std::string& inputPath) override;
    /// Description: Describes the request shutdown operation contract.
    void requestShutdown() override;
    /// Description: Describes the set remote snapshot cap operation contract.
    void setRemoteSnapshotCap(std::uint32_t maxPoints) override;
    /// Description: Describes the request reconnect operation contract.
    void requestReconnect() override;
    /// Description: Describes the configure remote connector operation contract.
    void configureRemoteConnector(const std::string& host, std::uint16_t port, bool autoStart,
                                  const std::string& serverExecutable) override;
    /// Description: Describes the is remote mode operation contract.
    bool isRemoteMode() const override;
    /// Description: Describes the get cached stats operation contract.
    SimulationStats getCachedStats() const override;
    /// Description: Describes the get stats operation contract.
    SimulationStats getStats() const override;
    /// Description: Describes the consume latest snapshot operation contract.
    std::optional<ConsumedSnapshot> consumeLatestSnapshot() override;
    /// Description: Describes the try consume snapshot operation contract.
    bool tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot) override;
    /// Description: Describes the snapshot pipeline state operation contract.
    SnapshotPipelineState snapshotPipelineState() const override;
    /// Description: Describes the link state label operation contract.
    std::string linkStateLabel() const override;
    /// Description: Describes the server owner label operation contract.
    std::string serverOwnerLabel() const override;
    /// Description: Describes the stats age ms operation contract.
    std::uint32_t statsAgeMs() const override;
    /// Description: Describes the snapshot age ms operation contract.
    std::uint32_t snapshotAgeMs() const override;

private:
    typedef std::chrono::steady_clock Clock;
    /// Description: Enumerates the supported SnapshotDropPolicyMode values.
    enum class SnapshotDropPolicyMode {
        LatestOnly,
        Paced
    };

    /// Description: Defines the SnapshotBufferEntry data or behavior contract.
    struct SnapshotBufferEntry final {
        std::vector<RenderParticle> particles;
        std::size_t sourceSize = 0u;
        Clock::time_point receivedAt{};
    };

    /// Description: Describes the invalidate cached snapshot operation contract.
    void invalidateCachedSnapshot();
    /// Description: Describes the poll loop operation contract.
    void pollLoop();
    /// Description: Describes the poll once operation contract.
    void pollOnce(bool pollSnapshot, bool pollStats);
    /// Description: Describes the age ms since operation contract.
    static std::uint32_t ageMsSince(Clock::time_point at, bool valid);
    /// Description: Describes the advance snapshot index operation contract.
    std::size_t advanceSnapshotIndex(std::size_t index) const;
    /// Description: Describes the queue snapshot operation contract.
    void queueSnapshot(std::vector<RenderParticle> snapshot, std::size_t sourceSize,
                       Clock::time_point now);
    ClientServerBridge _bridge;
    std::thread _pollThread;
    std::atomic<bool> _pollRunning;
    mutable std::mutex _dataMutex;
    SimulationStats _latestStats;
    std::vector<SnapshotBufferEntry> _snapshotRing;
    std::size_t _snapshotReadIndex;
    std::size_t _snapshotWriteIndex;
    std::size_t _snapshotCount;
    std::uint64_t _droppedSnapshots;
    std::uint32_t _lastSnapshotLatencyMs;
    SnapshotDropPolicyMode _snapshotDropPolicy;
    std::string _latestLinkLabel;
    std::string _latestOwnerLabel;
    Clock::time_point _lastStatsAt;
    Clock::time_point _lastSnapshotAt;
    bool _hasStats;
    bool _hasSnapshotEver;
    bool _hasDeliveredSnapshot;
};
} // namespace grav_client
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTRUNTIME_HPP_
