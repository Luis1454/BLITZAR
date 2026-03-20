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

class ClientRuntime final : public IClientRuntime {
    public:
        ClientRuntime(const std::string &configPath, const ClientTransportArgs &transport);
        ~ClientRuntime() override;

        bool start() override;
        void stop() override;

        void setPaused(bool paused) override;
        void togglePaused() override;
        void stepOnce() override;
        void setParticleCount(std::uint32_t particleCount) override;
        void setDt(float dt) override;
        void scaleDt(float factor) override;
        void requestReset() override;
        void requestRecover() override;
        void setSolverMode(const std::string &mode) override;
        void setIntegratorMode(const std::string &mode) override;
        void setPerformanceProfile(const std::string &profile) override;
        void setOctreeParameters(float theta, float softening) override;
        void setSphEnabled(bool enabled) override;
        void setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity) override;
        void setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps) override;
        void setSnapshotPublishPeriodMs(std::uint32_t periodMs) override;
        void setInitialStateConfig(const InitialStateConfig &config) override;
        void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit) override;
        void setExportDefaults(const std::string &directory, const std::string &format) override;
        void setInitialStateFile(const std::string &path, const std::string &format) override;
        void requestExportSnapshot(const std::string &outputPath, const std::string &format) override;
        void requestShutdown() override;
        void setRemoteSnapshotCap(std::uint32_t maxPoints) override;
        void requestReconnect() override;
        void configureRemoteConnector(
            const std::string &host,
            std::uint16_t port,
            bool autoStart,
            const std::string &serverExecutable
        ) override;
        bool isRemoteMode() const override;

        SimulationStats getCachedStats() const override;
        SimulationStats getStats() const override;
        std::optional<ConsumedSnapshot> consumeLatestSnapshot() override;
        bool tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot) override;
        SnapshotPipelineState snapshotPipelineState() const override;
        std::string linkStateLabel() const override;
        std::string serverOwnerLabel() const override;
        std::uint32_t statsAgeMs() const override;
        std::uint32_t snapshotAgeMs() const override;

    private:
        typedef std::chrono::steady_clock Clock;
        enum class SnapshotDropPolicyMode {
            LatestOnly,
            Paced
        };
        struct SnapshotBufferEntry final {
            std::vector<RenderParticle> particles;
            std::size_t sourceSize = 0u;
            Clock::time_point receivedAt{};
        };
        void invalidateCachedSnapshot();
        void pollLoop();
        void pollOnce(bool pollSnapshot, bool pollStats);
        static std::uint32_t ageMsSince(Clock::time_point at, bool valid);
        std::size_t advanceSnapshotIndex(std::size_t index) const;
        void queueSnapshot(std::vector<RenderParticle> snapshot, Clock::time_point now);

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
