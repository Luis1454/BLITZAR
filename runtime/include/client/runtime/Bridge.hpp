/*
 * @file runtime/include/client/runtime/Bridge.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTSERVERBRIDGE_HPP_
#define BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTSERVERBRIDGE_HPP_
#include "Constants.hpp"
#include "client/runtime/BridgeState.hpp"
#include "protocol/client/Client.hpp"
#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace bltzr_client {
extern const std::uint32_t kRemoteTimeoutMinMs;
extern const std::uint32_t kRemoteTimeoutMaxMs;
extern const std::uint32_t kRemoteCommandTimeoutMsDefault;
extern const std::uint32_t kRemoteStatusTimeoutMsDefault;
extern const std::uint32_t kRemoteSnapshotTimeoutMsDefault;
std::uint32_t clampRemoteTimeoutMs(std::uint32_t timeoutMs);

struct TransportArgs {
    std::string remoteHost = kDefaultLoopbackHost;
    std::uint16_t remotePort = kDefaultServerPort;
    bool remoteAutoStart = false;
    std::string serverExecutable;
    std::string remoteAuthToken;
    std::uint32_t remoteCommandTimeoutMs = kRemoteCommandTimeoutMsDefault;
    std::uint32_t remoteStatusTimeoutMs = kRemoteStatusTimeoutMsDefault;
    std::uint32_t remoteSnapshotTimeoutMs = kRemoteSnapshotTimeoutMsDefault;
};
enum class LinkState {
    Connected,
    Reconnecting
};
bool splitTransportArgs(const std::vector<std::string_view>& rawArgs,
                              std::vector<std::string_view>& filteredArgs,
                              TransportArgs& transport, std::ostream& warnings);

class Bridge {
public:
    Bridge(const std::string& configPath, std::string remoteHost,
                       std::uint16_t remotePort, bool remoteAutoStart, std::string serverExecutable,
                       std::string remoteAuthToken, std::uint32_t remoteCommandTimeoutMs,
                       std::uint32_t remoteStatusTimeoutMs, std::uint32_t remoteSnapshotTimeoutMs);
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
    void setSolverMode(const std::string& mode);
    void setIntegratorMode(const std::string& mode);
    void setPerformanceProfile(const std::string& profile);
    void setOctreeParameters(float theta, float softening);
    void setSphEnabled(bool enabled);
    void setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                          float viscosity);
    void setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps);
    void setSnapshotPublishPeriodMs(std::uint32_t periodMs);
    void setInitialStateConfig(const InitialStateConfig& config);
    void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit);
    void setGpuTelemetryEnabled(bool enabled);
    void setExportDefaults(const std::string& directory, const std::string& format);
    void setInitialStateFile(const std::string& path, const std::string& format);
    void requestExportSnapshot(const std::string& outputPath, const std::string& format);
    void requestSaveCheckpoint(const std::string& outputPath);
    void requestLoadCheckpoint(const std::string& inputPath);
    void requestShutdown();
    void configureRemoteConnector(const std::string& host, std::uint16_t port, bool autoStart,
                                  const std::string& serverExecutable);
    bool tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot,
                            std::size_t* outSourceSize = nullptr);
    SimulationStats getStats();
    void setRemoteSnapshotCap(std::uint32_t maxPoints);
    void requestReconnect();
    bool isRemoteMode() const;
    bool launchedByClient() const;
    LinkState linkState() const;
    std::string_view linkStateLabel() const;
    std::string_view serverOwnerLabel() const;

private:
    static std::string jsonEscape(const std::string& value);
    static SimulationStats fromRemoteStatus(const bltzr_protocol::ClientStatus& status);
    bool sendRemoteNow(const std::string& cmd, const std::string& fields = "");
    bool sendOrQueueRemote(const std::string& cmd, const std::string& fields = "");
    bool ensureRemoteConnected(bool forceLog);
    void markRemoteDisconnected(const std::string& context, const std::string& reason);
    bool shouldAutoStartRemoteServer() const;
    void tryAutoStartRemoteServer();
    static bool isLoopbackHost(std::string_view host);
    void queuePendingRemoteCommand(const std::string& cmd, const std::string& fields);
    void flushPendingRemoteCommands();
    void refreshRemoteStats();
    std::string _configPath;
    std::string _remoteHost;
    std::uint16_t _remotePort;
    bool _remoteAutoStart;
    std::string _serverExecutable;
    std::string _remoteAuthToken;
    bltzr_protocol::Client _remoteClient;
    bool _remoteLaunchAttempted;
    BridgeState _runtimeState;
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
} // namespace bltzr_client
#endif // BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTSERVERBRIDGE_HPP_
