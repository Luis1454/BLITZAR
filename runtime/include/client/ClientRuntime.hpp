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
    /// Description: Executes the ClientRuntime operation.
    ClientRuntime(const std::string& configPath, const ClientTransportArgs& transport);
    /// Description: Releases resources owned by ClientRuntime.
    ~ClientRuntime() override;
    /// Description: Executes the start operation.
    bool start() override;
    /// Description: Executes the stop operation.
    void stop() override;
    /// Description: Executes the setPaused operation.
    void setPaused(bool paused) override;
    /// Description: Executes the togglePaused operation.
    void togglePaused() override;
    /// Description: Executes the stepOnce operation.
    void stepOnce() override;
    /// Description: Executes the setParticleCount operation.
    void setParticleCount(std::uint32_t particleCount) override;
    /// Description: Executes the setDt operation.
    void setDt(float dt) override;
    /// Description: Executes the scaleDt operation.
    void scaleDt(float factor) override;
    /// Description: Executes the requestReset operation.
    void requestReset() override;
    /// Description: Executes the requestRecover operation.
    void requestRecover() override;
    /// Description: Executes the setSolverMode operation.
    void setSolverMode(const std::string& mode) override;
    /// Description: Executes the setIntegratorMode operation.
    void setIntegratorMode(const std::string& mode) override;
    /// Description: Executes the setPerformanceProfile operation.
    void setPerformanceProfile(const std::string& profile) override;
    /// Description: Executes the setOctreeParameters operation.
    void setOctreeParameters(float theta, float softening) override;
    /// Description: Executes the setSphEnabled operation.
    void setSphEnabled(bool enabled) override;
    void setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                          float viscosity) override;
    /// Description: Executes the setSubstepPolicy operation.
    void setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps) override;
    /// Description: Executes the setSnapshotPublishPeriodMs operation.
    void setSnapshotPublishPeriodMs(std::uint32_t periodMs) override;
    /// Description: Executes the setInitialStateConfig operation.
    void setInitialStateConfig(const InitialStateConfig& config) override;
    /// Description: Executes the setEnergyMeasurementConfig operation.
    void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit) override;
    /// Description: Executes the setGpuTelemetryEnabled operation.
    void setGpuTelemetryEnabled(bool enabled) override;
    /// Description: Executes the setExportDefaults operation.
    void setExportDefaults(const std::string& directory, const std::string& format) override;
    /// Description: Executes the setInitialStateFile operation.
    void setInitialStateFile(const std::string& path, const std::string& format) override;
    /// Description: Executes the requestExportSnapshot operation.
    void requestExportSnapshot(const std::string& outputPath, const std::string& format) override;
    /// Description: Executes the requestSaveCheckpoint operation.
    void requestSaveCheckpoint(const std::string& outputPath) override;
    /// Description: Executes the requestLoadCheckpoint operation.
    void requestLoadCheckpoint(const std::string& inputPath) override;
    /// Description: Executes the requestShutdown operation.
    void requestShutdown() override;
    /// Description: Executes the setRemoteSnapshotCap operation.
    void setRemoteSnapshotCap(std::uint32_t maxPoints) override;
    /// Description: Executes the requestReconnect operation.
    void requestReconnect() override;
    void configureRemoteConnector(const std::string& host, std::uint16_t port, bool autoStart,
                                  const std::string& serverExecutable) override;
    /// Description: Executes the isRemoteMode operation.
    bool isRemoteMode() const override;
    /// Description: Executes the getCachedStats operation.
    SimulationStats getCachedStats() const override;
    /// Description: Executes the getStats operation.
    SimulationStats getStats() const override;
    /// Description: Executes the consumeLatestSnapshot operation.
    std::optional<ConsumedSnapshot> consumeLatestSnapshot() override;
    /// Description: Executes the tryConsumeSnapshot operation.
    bool tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot) override;
    /// Description: Executes the snapshotPipelineState operation.
    SnapshotPipelineState snapshotPipelineState() const override;
    /// Description: Executes the linkStateLabel operation.
    std::string linkStateLabel() const override;
    /// Description: Executes the serverOwnerLabel operation.
    std::string serverOwnerLabel() const override;
    /// Description: Executes the statsAgeMs operation.
    std::uint32_t statsAgeMs() const override;
    /// Description: Executes the snapshotAgeMs operation.
    std::uint32_t snapshotAgeMs() const override;

private:
    typedef std::chrono::steady_clock Clock;
    /// Description: Enumerates the supported SnapshotDropPolicyMode values.
    enum class SnapshotDropPolicyMode { LatestOnly, Paced };
    /// Description: Defines the SnapshotBufferEntry data or behavior contract.
    struct SnapshotBufferEntry final {
        std::vector<RenderParticle> particles;
        std::size_t sourceSize = 0u;
        Clock::time_point receivedAt{};
    };
    /// Description: Executes the invalidateCachedSnapshot operation.
    void invalidateCachedSnapshot();
    /// Description: Executes the pollLoop operation.
    void pollLoop();
    /// Description: Executes the pollOnce operation.
    void pollOnce(bool pollSnapshot, bool pollStats);
    /// Description: Executes the ageMsSince operation.
    static std::uint32_t ageMsSince(Clock::time_point at, bool valid);
    /// Description: Executes the advanceSnapshotIndex operation.
    std::size_t advanceSnapshotIndex(std::size_t index) const;
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
