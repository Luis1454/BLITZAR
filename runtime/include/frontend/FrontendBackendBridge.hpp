#ifndef GRAVITY_RUNTIME_INCLUDE_FRONTEND_FRONTENDBACKENDBRIDGE_HPP_
#define GRAVITY_RUNTIME_INCLUDE_FRONTEND_FRONTENDBACKENDBRIDGE_HPP_

#include "protocol/BackendClient.hpp"
#include "frontend/ILocalBackend.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace grav_frontend {

extern const std::uint32_t kFrontendRemoteTimeoutMinMs;
extern const std::uint32_t kFrontendRemoteTimeoutMaxMs;
extern const std::uint32_t kFrontendRemoteCommandTimeoutMsDefault;
extern const std::uint32_t kFrontendRemoteStatusTimeoutMsDefault;
extern const std::uint32_t kFrontendRemoteSnapshotTimeoutMsDefault;

std::uint32_t clampFrontendRemoteTimeoutMs(std::uint32_t timeoutMs);

struct FrontendTransportArgs {
    bool remoteMode = false;
    std::string remoteHost = "127.0.0.1";
    std::uint16_t remotePort = 4545u;
    bool remoteAutoStart = false;
    std::string backendExecutable;
    std::string remoteAuthToken;
    std::uint32_t remoteCommandTimeoutMs = kFrontendRemoteCommandTimeoutMsDefault;
    std::uint32_t remoteStatusTimeoutMs = kFrontendRemoteStatusTimeoutMsDefault;
    std::uint32_t remoteSnapshotTimeoutMs = kFrontendRemoteSnapshotTimeoutMsDefault;
};

enum class FrontendLinkState {
    LocalEmbedded,
    Connected,
    Reconnecting
};

bool splitFrontendTransportArgs(
    const std::vector<std::string_view> &rawArgs,
    std::vector<std::string_view> &filteredArgs,
    FrontendTransportArgs &transport,
    std::ostream &warnings
);

class FrontendBackendBridge {
    public:
        typedef std::function<std::unique_ptr<ILocalBackend>(const std::string &)> LocalBackendFactory;
        FrontendBackendBridge(
            const std::string &configPath,
            bool remoteMode,
            std::string remoteHost,
            std::uint16_t remotePort,
            bool remoteAutoStart,
            std::string backendExecutable,
            std::string remoteAuthToken,
            std::uint32_t remoteCommandTimeoutMs,
            std::uint32_t remoteStatusTimeoutMs,
            std::uint32_t remoteSnapshotTimeoutMs,
            LocalBackendFactory localBackendFactory
        );

        bool start();
        void stop();

        void setPaused(bool paused);
        void togglePaused();
        void stepOnce();

        void setParticleCount(std::uint32_t particleCount);
        void setDt(float dt);
        void scaleDt(float factor);

        void requestReset();
        void requestRecover();
        void setSolverMode(const std::string &mode);
        void setIntegratorMode(const std::string &mode);
        void setOctreeParameters(float theta, float softening);
        void setSphEnabled(bool enabled);
        void setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity);
        void setInitialStateConfig(const InitialStateConfig &config);
        void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit);
        void setExportDefaults(const std::string &directory, const std::string &format);
        void setInitialStateFile(const std::string &path, const std::string &format);
        void requestExportSnapshot(const std::string &outputPath, const std::string &format);
        void requestShutdown();
        void configureRemoteConnector(
            const std::string &host,
            std::uint16_t port,
            bool autoStart,
            const std::string &backendExecutable
        );

        bool tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot);
        SimulationStats getStats();
        void setRemoteSnapshotCap(std::uint32_t maxPoints);
        void requestReconnect();
        bool isRemoteMode() const;
        bool launchedByFrontend() const;
        FrontendLinkState linkState() const;
        std::string_view linkStateLabel() const;
        std::string_view backendOwnerLabel() const;

    private:
        static std::string jsonEscape(const std::string &value);
        static SimulationStats fromRemoteStatus(const BackendClientStatus &status);
        bool sendRemoteNow(const std::string &cmd, const std::string &fields = "");
        bool sendOrQueueRemote(const std::string &cmd, const std::string &fields = "");
        bool ensureRemoteConnected(bool forceLog);
        void markRemoteDisconnected(const std::string &context, const std::string &reason);
        bool shouldAutoStartRemoteBackend() const;
        void tryAutoStartRemoteBackend();
        static bool isLoopbackHost(std::string_view host);
        void queuePendingRemoteCommand(const std::string &cmd, const std::string &fields);
        void flushPendingRemoteCommands();
        void refreshRemoteStats();

        std::string _configPath;
        bool _remoteMode;
        LocalBackendFactory _localBackendFactory;
        std::string _remoteHost;
        std::uint16_t _remotePort;
        bool _remoteAutoStart;
        std::string _backendExecutable;
        std::string _remoteAuthToken;
        std::unique_ptr<ILocalBackend> _localBackend;
        BackendClient _remoteClient;
        bool _remoteConnected;
        bool _remoteLaunchAttempted;
        bool _remoteBackendLaunched;
        bool _pendingQueueDropWarned;
        std::vector<std::pair<std::string, std::string>> _pendingRemoteCommands;
        std::uint32_t _remoteSnapshotCap;
        SimulationStats _cachedStats;
        bool _warnedRemoteInitialConfig;
        std::string _defaultExportFormat;
        std::chrono::steady_clock::time_point _lastReconnectAttempt;
        std::chrono::steady_clock::time_point _lastReconnectErrorLog;
        std::chrono::steady_clock::time_point _lastRemoteErrorLog;
        std::chrono::milliseconds _reconnectRetryDelay;
        std::uint32_t _remoteCommandTimeoutMs;
        std::uint32_t _remoteStatusTimeoutMs;
        std::uint32_t _remoteSnapshotTimeoutMs;
        mutable std::recursive_mutex _mutex;
};

} // namespace grav_frontend



#endif // GRAVITY_RUNTIME_INCLUDE_FRONTEND_FRONTENDBACKENDBRIDGE_HPP_
