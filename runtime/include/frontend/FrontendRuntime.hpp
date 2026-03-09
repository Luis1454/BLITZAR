#pragma once

#include "frontend/FrontendBackendBridge.hpp"
#include "frontend/IFrontendRuntime.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace grav_frontend {

class FrontendRuntime final : public IFrontendRuntime {
    public:
        FrontendRuntime(const std::string &configPath, const FrontendTransportArgs &transport);
        ~FrontendRuntime() override;

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
        void setOctreeParameters(float theta, float softening) override;
        void setSphEnabled(bool enabled) override;
        void setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity) override;
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
            const std::string &backendExecutable
        ) override;
        bool isRemoteMode() const override;

        SimulationStats getCachedStats() const override;
        SimulationStats getStats() const override;
        std::optional<ConsumedSnapshot> consumeLatestSnapshot() override;
        bool tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot) override;
        std::string linkStateLabel() const override;
        std::string backendOwnerLabel() const override;
        std::uint32_t statsAgeMs() const override;
        std::uint32_t snapshotAgeMs() const override;

    private:
        typedef std::chrono::steady_clock Clock;
        void pollLoop();
        void pollOnce(bool pollSnapshot, bool pollStats);
        static std::uint32_t ageMsSince(Clock::time_point at, bool valid);

        FrontendBackendBridge _bridge;
        std::thread _pollThread;
        std::atomic<bool> _pollRunning;
        mutable std::mutex _dataMutex;
        SimulationStats _latestStats;
        std::vector<RenderParticle> _latestSnapshot;
        bool _hasNewSnapshot;
        std::size_t _latestSnapshotSize;
        std::string _latestLinkLabel;
        std::string _latestOwnerLabel;
        Clock::time_point _lastStatsAt;
        Clock::time_point _lastSnapshotAt;
        bool _hasStats;
        bool _hasSnapshotEver;
};

} // namespace grav_frontend


