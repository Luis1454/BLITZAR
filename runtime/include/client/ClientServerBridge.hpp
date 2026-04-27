// File: runtime/include/client/ClientServerBridge.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTSERVERBRIDGE_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTSERVERBRIDGE_HPP_
#include "client/RustRuntimeBridgeState.hpp"
#include "protocol/ServerClient.hpp"
#include <chrono>
#include <cstdint>
#include <iosfwd>
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
/// Description: Executes the clampClientRemoteTimeoutMs operation.
std::uint32_t clampClientRemoteTimeoutMs(std::uint32_t timeoutMs);
/// Description: Defines the ClientTransportArgs data or behavior contract.
struct ClientTransportArgs {
    std::string remoteHost = "127.0.0.1";
    std::uint16_t remotePort = 4545u;
    bool remoteAutoStart = false;
    std::string serverExecutable;
    std::string remoteAuthToken;
    std::uint32_t remoteCommandTimeoutMs = kClientRemoteCommandTimeoutMsDefault;
    std::uint32_t remoteStatusTimeoutMs = kClientRemoteStatusTimeoutMsDefault;
    std::uint32_t remoteSnapshotTimeoutMs = kClientRemoteSnapshotTimeoutMsDefault;
};
/// Description: Enumerates the supported ClientLinkState values.
enum class ClientLinkState { Connected, Reconnecting };
bool splitClientTransportArgs(const std::vector<std::string_view>& rawArgs,
                              std::vector<std::string_view>& filteredArgs,
                              ClientTransportArgs& transport, std::ostream& warnings);
/// Description: Defines the ClientServerBridge data or behavior contract.
class ClientServerBridge {
public:
    ClientServerBridge(const std::string& configPath, std::string remoteHost,
                       std::uint16_t remotePort, bool remoteAutoStart, std::string serverExecutable,
                       std::string remoteAuthToken, std::uint32_t remoteCommandTimeoutMs,
                       std::uint32_t remoteStatusTimeoutMs, std::uint32_t remoteSnapshotTimeoutMs);
    /// Description: Executes the start operation.
    bool start();
    /// Description: Executes the stop operation.
    void stop();
    /// Description: Executes the setPaused operation.
    void setPaused(bool paused);
    /// Description: Executes the togglePaused operation.
    void togglePaused();
    /// Description: Executes the stepOnce operation.
    void stepOnce();
    /// Description: Executes the setParticleCount operation.
    void setParticleCount(std::uint32_t particleCount);
    /// Description: Executes the setDt operation.
    void setDt(float dt);
    /// Description: Executes the scaleDt operation.
    void scaleDt(float factor);
    /// Description: Executes the requestReset operation.
    void requestReset();
    /// Description: Executes the requestRecover operation.
    void requestRecover();
    /// Description: Executes the setSolverMode operation.
    void setSolverMode(const std::string& mode);
    /// Description: Executes the setIntegratorMode operation.
    void setIntegratorMode(const std::string& mode);
    /// Description: Executes the setPerformanceProfile operation.
    void setPerformanceProfile(const std::string& profile);
    /// Description: Executes the setOctreeParameters operation.
    void setOctreeParameters(float theta, float softening);
    /// Description: Executes the setSphEnabled operation.
    void setSphEnabled(bool enabled);
    void setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                          float viscosity);
    /// Description: Executes the setSubstepPolicy operation.
    void setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps);
    /// Description: Executes the setSnapshotPublishPeriodMs operation.
    void setSnapshotPublishPeriodMs(std::uint32_t periodMs);
    /// Description: Executes the setInitialStateConfig operation.
    void setInitialStateConfig(const InitialStateConfig& config);
    /// Description: Executes the setEnergyMeasurementConfig operation.
    void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit);
    /// Description: Executes the setGpuTelemetryEnabled operation.
    void setGpuTelemetryEnabled(bool enabled);
    /// Description: Executes the setExportDefaults operation.
    void setExportDefaults(const std::string& directory, const std::string& format);
    /// Description: Executes the setInitialStateFile operation.
    void setInitialStateFile(const std::string& path, const std::string& format);
    /// Description: Executes the requestExportSnapshot operation.
    void requestExportSnapshot(const std::string& outputPath, const std::string& format);
    /// Description: Executes the requestSaveCheckpoint operation.
    void requestSaveCheckpoint(const std::string& outputPath);
    /// Description: Executes the requestLoadCheckpoint operation.
    void requestLoadCheckpoint(const std::string& inputPath);
    /// Description: Executes the requestShutdown operation.
    void requestShutdown();
    void configureRemoteConnector(const std::string& host, std::uint16_t port, bool autoStart,
                                  const std::string& serverExecutable);
    bool tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot,
                            std::size_t* outSourceSize = nullptr);
    /// Description: Executes the getStats operation.
    SimulationStats getStats();
    /// Description: Executes the setRemoteSnapshotCap operation.
    void setRemoteSnapshotCap(std::uint32_t maxPoints);
    /// Description: Executes the requestReconnect operation.
    void requestReconnect();
    /// Description: Executes the isRemoteMode operation.
    bool isRemoteMode() const;
    /// Description: Executes the launchedByClient operation.
    bool launchedByClient() const;
    /// Description: Executes the linkState operation.
    ClientLinkState linkState() const;
    /// Description: Executes the linkStateLabel operation.
    std::string_view linkStateLabel() const;
    /// Description: Executes the serverOwnerLabel operation.
    std::string_view serverOwnerLabel() const;

private:
    /// Description: Executes the jsonEscape operation.
    static std::string jsonEscape(const std::string& value);
    /// Description: Executes the fromRemoteStatus operation.
    static SimulationStats fromRemoteStatus(const ServerClientStatus& status);
    /// Description: Executes the sendRemoteNow operation.
    bool sendRemoteNow(const std::string& cmd, const std::string& fields = "");
    /// Description: Executes the sendOrQueueRemote operation.
    bool sendOrQueueRemote(const std::string& cmd, const std::string& fields = "");
    /// Description: Executes the ensureRemoteConnected operation.
    bool ensureRemoteConnected(bool forceLog);
    /// Description: Executes the markRemoteDisconnected operation.
    void markRemoteDisconnected(const std::string& context, const std::string& reason);
    /// Description: Executes the shouldAutoStartRemoteServer operation.
    bool shouldAutoStartRemoteServer() const;
    /// Description: Executes the tryAutoStartRemoteServer operation.
    void tryAutoStartRemoteServer();
    /// Description: Executes the isLoopbackHost operation.
    static bool isLoopbackHost(std::string_view host);
    /// Description: Executes the queuePendingRemoteCommand operation.
    void queuePendingRemoteCommand(const std::string& cmd, const std::string& fields);
    /// Description: Executes the flushPendingRemoteCommands operation.
    void flushPendingRemoteCommands();
    /// Description: Executes the refreshRemoteStats operation.
    void refreshRemoteStats();
    std::string _configPath;
    std::string _remoteHost;
    std::uint16_t _remotePort;
    bool _remoteAutoStart;
    std::string _serverExecutable;
    std::string _remoteAuthToken;
    ServerClient _remoteClient;
    bool _remoteLaunchAttempted;
    RustRuntimeBridgeState _runtimeState;
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
