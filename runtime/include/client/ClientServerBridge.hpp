#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTSERVERBRIDGE_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTSERVERBRIDGE_HPP_

#include "protocol/ServerClient.hpp"
#include "client/ILocalServer.hpp"

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

namespace grav_client {

extern const std::uint32_t kClientRemoteTimeoutMinMs;
extern const std::uint32_t kClientRemoteTimeoutMaxMs;
extern const std::uint32_t kClientRemoteCommandTimeoutMsDefault;
extern const std::uint32_t kClientRemoteStatusTimeoutMsDefault;
extern const std::uint32_t kClientRemoteSnapshotTimeoutMsDefault;

std::uint32_t clampClientRemoteTimeoutMs(std::uint32_t timeoutMs);

struct ClientTransportArgs {
    bool remoteMode = false;
    std::string remoteHost = "127.0.0.1";
    std::uint16_t remotePort = 4545u;
    bool remoteAutoStart = false;
    std::string serverExecutable;
    std::string remoteAuthToken;
    std::uint32_t remoteCommandTimeoutMs = kClientRemoteCommandTimeoutMsDefault;
    std::uint32_t remoteStatusTimeoutMs = kClientRemoteStatusTimeoutMsDefault;
    std::uint32_t remoteSnapshotTimeoutMs = kClientRemoteSnapshotTimeoutMsDefault;
};

enum class ClientLinkState {
    LocalEmbedded,
    Connected,
    Reconnecting
};

bool splitClientTransportArgs(
    const std::vector<std::string_view> &rawArgs,
    std::vector<std::string_view> &filteredArgs,
    ClientTransportArgs &transport,
    std::ostream &warnings
);

class ClientServerBridge {
    public:
        typedef std::function<std::unique_ptr<ILocalServer>(const std::string &)> LocalServerFactory;
        ClientServerBridge(
            const std::string &configPath,
            bool remoteMode,
            std::string remoteHost,
            std::uint16_t remotePort,
            bool remoteAutoStart,
            std::string serverExecutable,
            std::string remoteAuthToken,
            std::uint32_t remoteCommandTimeoutMs,
            std::uint32_t remoteStatusTimeoutMs,
            std::uint32_t remoteSnapshotTimeoutMs,
            LocalServerFactory localServerFactory
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
        void setPerformanceProfile(const std::string &profile);
        void setOctreeParameters(float theta, float softening);
        void setSphEnabled(bool enabled);
        void setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity);
        void setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps);
        void setSnapshotPublishPeriodMs(std::uint32_t periodMs);
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
            const std::string &serverExecutable
        );

        bool tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot);
        SimulationStats getStats();
        void setRemoteSnapshotCap(std::uint32_t maxPoints);
        void requestReconnect();
        bool isRemoteMode() const;
        bool launchedByClient() const;
        ClientLinkState linkState() const;
        std::string_view linkStateLabel() const;
        std::string_view serverOwnerLabel() const;

    private:
        static std::string jsonEscape(const std::string &value);
        static SimulationStats fromRemoteStatus(const ServerClientStatus &status);
        bool sendRemoteNow(const std::string &cmd, const std::string &fields = "");
        bool sendOrQueueRemote(const std::string &cmd, const std::string &fields = "");
        bool ensureRemoteConnected(bool forceLog);
        void markRemoteDisconnected(const std::string &context, const std::string &reason);
        bool shouldAutoStartRemoteServer() const;
        void tryAutoStartRemoteServer();
        static bool isLoopbackHost(std::string_view host);
        void queuePendingRemoteCommand(const std::string &cmd, const std::string &fields);
        void flushPendingRemoteCommands();
        void refreshRemoteStats();

        std::string _configPath;
        bool _remoteMode;
        LocalServerFactory _localServerFactory;
        std::string _remoteHost;
        std::uint16_t _remotePort;
        bool _remoteAutoStart;
        std::string _serverExecutable;
        std::string _remoteAuthToken;
        std::unique_ptr<ILocalServer> _localServer;
        ServerClient _remoteClient;
        bool _remoteConnected;
        bool _remoteLaunchAttempted;
        bool _remoteServerLaunched;
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

} // namespace grav_client



#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTSERVERBRIDGE_HPP_
