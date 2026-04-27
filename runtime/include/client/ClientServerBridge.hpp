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
enum class ClientLinkState {
    Connected,
    Reconnecting
};
/// Description: Describes the split client transport args operation contract.
bool splitClientTransportArgs(const std::vector<std::string_view>& rawArgs,
                              std::vector<std::string_view>& filteredArgs,
                              ClientTransportArgs& transport, std::ostream& warnings);

/// Description: Defines the ClientServerBridge data or behavior contract.
class ClientServerBridge {
public:
    /// Description: Describes the client server bridge operation contract.
    ClientServerBridge(const std::string& configPath, std::string remoteHost,
                       std::uint16_t remotePort, bool remoteAutoStart, std::string serverExecutable,
                       std::string remoteAuthToken, std::uint32_t remoteCommandTimeoutMs,
                       std::uint32_t remoteStatusTimeoutMs, std::uint32_t remoteSnapshotTimeoutMs);
    /// Description: Describes the start operation contract.
    bool start();
    /// Description: Describes the stop operation contract.
    void stop();
    /// Description: Describes the set paused operation contract.
    void setPaused(bool paused);
    /// Description: Describes the toggle paused operation contract.
    void togglePaused();
    /// Description: Describes the step once operation contract.
    void stepOnce();
    /// Description: Describes the set particle count operation contract.
    void setParticleCount(std::uint32_t particleCount);
    /// Description: Describes the set dt operation contract.
    void setDt(float dt);
    /// Description: Describes the scale dt operation contract.
    void scaleDt(float factor);
    /// Description: Describes the request reset operation contract.
    void requestReset();
    /// Description: Describes the request recover operation contract.
    void requestRecover();
    /// Description: Describes the set solver mode operation contract.
    void setSolverMode(const std::string& mode);
    /// Description: Describes the set integrator mode operation contract.
    void setIntegratorMode(const std::string& mode);
    /// Description: Describes the set performance profile operation contract.
    void setPerformanceProfile(const std::string& profile);
    /// Description: Describes the set octree parameters operation contract.
    void setOctreeParameters(float theta, float softening);
    /// Description: Describes the set sph enabled operation contract.
    void setSphEnabled(bool enabled);
    /// Description: Describes the set sph parameters operation contract.
    void setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                          float viscosity);
    /// Description: Describes the set substep policy operation contract.
    void setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps);
    /// Description: Describes the set snapshot publish period ms operation contract.
    void setSnapshotPublishPeriodMs(std::uint32_t periodMs);
    /// Description: Describes the set initial state config operation contract.
    void setInitialStateConfig(const InitialStateConfig& config);
    /// Description: Describes the set energy measurement config operation contract.
    void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit);
    /// Description: Describes the set gpu telemetry enabled operation contract.
    void setGpuTelemetryEnabled(bool enabled);
    /// Description: Describes the set export defaults operation contract.
    void setExportDefaults(const std::string& directory, const std::string& format);
    /// Description: Describes the set initial state file operation contract.
    void setInitialStateFile(const std::string& path, const std::string& format);
    /// Description: Describes the request export snapshot operation contract.
    void requestExportSnapshot(const std::string& outputPath, const std::string& format);
    /// Description: Describes the request save checkpoint operation contract.
    void requestSaveCheckpoint(const std::string& outputPath);
    /// Description: Describes the request load checkpoint operation contract.
    void requestLoadCheckpoint(const std::string& inputPath);
    /// Description: Describes the request shutdown operation contract.
    void requestShutdown();
    /// Description: Describes the configure remote connector operation contract.
    void configureRemoteConnector(const std::string& host, std::uint16_t port, bool autoStart,
                                  const std::string& serverExecutable);
    /// Description: Describes the try consume snapshot operation contract.
    bool tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot,
                            std::size_t* outSourceSize = nullptr);
    /// Description: Describes the get stats operation contract.
    SimulationStats getStats();
    /// Description: Describes the set remote snapshot cap operation contract.
    void setRemoteSnapshotCap(std::uint32_t maxPoints);
    /// Description: Describes the request reconnect operation contract.
    void requestReconnect();
    /// Description: Describes the is remote mode operation contract.
    bool isRemoteMode() const;
    /// Description: Describes the launched by client operation contract.
    bool launchedByClient() const;
    /// Description: Describes the link state operation contract.
    ClientLinkState linkState() const;
    /// Description: Describes the link state label operation contract.
    std::string_view linkStateLabel() const;
    /// Description: Describes the server owner label operation contract.
    std::string_view serverOwnerLabel() const;

private:
    /// Description: Describes the json escape operation contract.
    static std::string jsonEscape(const std::string& value);
    /// Description: Describes the from remote status operation contract.
    static SimulationStats fromRemoteStatus(const ServerClientStatus& status);
    /// Description: Describes the send remote now operation contract.
    bool sendRemoteNow(const std::string& cmd, const std::string& fields = "");
    /// Description: Describes the send or queue remote operation contract.
    bool sendOrQueueRemote(const std::string& cmd, const std::string& fields = "");
    /// Description: Describes the ensure remote connected operation contract.
    bool ensureRemoteConnected(bool forceLog);
    /// Description: Describes the mark remote disconnected operation contract.
    void markRemoteDisconnected(const std::string& context, const std::string& reason);
    /// Description: Describes the should auto start remote server operation contract.
    bool shouldAutoStartRemoteServer() const;
    /// Description: Describes the try auto start remote server operation contract.
    void tryAutoStartRemoteServer();
    /// Description: Describes the is loopback host operation contract.
    static bool isLoopbackHost(std::string_view host);
    /// Description: Describes the queue pending remote command operation contract.
    void queuePendingRemoteCommand(const std::string& cmd, const std::string& fields);
    /// Description: Describes the flush pending remote commands operation contract.
    void flushPendingRemoteCommands();
    /// Description: Describes the refresh remote stats operation contract.
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
