// File: runtime/include/protocol/ServerClient.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERCLIENT_HPP_
#define GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERCLIENT_HPP_
#include "protocol/ServerJsonCodec.hpp"
#include <cstdint>
#include <string>
#include <vector>
/// Description: Defines the ServerClientResponse data or behavior contract.
struct ServerClientResponse {
    bool ok = false;
    std::string raw;
    std::string error;
};
/// Description: Defines the ServerClientStatus data or behavior contract.
struct ServerClientStatus {
    std::uint64_t steps = 0;
    float dt = 0.0f;
    float totalTime = 0.0f;
    bool paused = false;
    bool faulted = false;
    std::uint64_t faultStep = 0;
    std::string faultReason;
    bool sphEnabled = false;
    float serverFps = 0.0f;
    std::string performanceProfile;
    float substepTargetDt = 0.0f;
    float substepDt = 0.0f;
    std::uint32_t substeps = 0;
    std::uint32_t maxSubsteps = 0;
    std::uint32_t snapshotPublishPeriodMs = 0;
    std::uint32_t particleCount = 0;
    std::string solver;
    std::string integrator;
    float kineticEnergy = 0.0f;
    float potentialEnergy = 0.0f;
    float thermalEnergy = 0.0f;
    float radiatedEnergy = 0.0f;
    float totalEnergy = 0.0f;
    float energyDriftPct = 0.0f;
    bool energyEstimated = false;
    bool gpuTelemetryEnabled = false;
    bool gpuTelemetryAvailable = false;
    float gpuKernelMs = 0.0f;
    float gpuCopyMs = 0.0f;
    std::uint64_t gpuVramUsedBytes = 0u;
    std::uint64_t gpuVramTotalBytes = 0u;
    std::uint32_t exportQueueDepth = 0u;
    bool exportActive = false;
    std::uint64_t exportCompletedCount = 0u;
    std::uint64_t exportFailedCount = 0u;
    std::string exportLastState;
    std::string exportLastPath;
    std::string exportLastMessage;
};
/// Description: Defines the ServerClient data or behavior contract.
class ServerClient {
public:
    /// Description: Executes the ServerClient operation.
    ServerClient();
    /// Description: Releases resources owned by ServerClient.
    ~ServerClient();
    /// Description: Executes the connect operation.
    bool connect(const std::string& host, std::uint16_t port);
    /// Description: Executes the setSocketTimeoutMs operation.
    void setSocketTimeoutMs(int timeoutMs);
    /// Description: Executes the socketTimeoutMs operation.
    int socketTimeoutMs() const;
    /// Description: Executes the setAuthToken operation.
    void setAuthToken(std::string token);
    /// Description: Executes the disconnect operation.
    void disconnect();
    /// Description: Executes the isConnected operation.
    bool isConnected() const;
    /// Description: Executes the sendJson operation.
    ServerClientResponse sendJson(const std::string& jsonLine);
    /// Description: Executes the sendCommand operation.
    ServerClientResponse sendCommand(const std::string& cmd, const std::string& fieldsJson = "");
    /// Description: Executes the getStatus operation.
    ServerClientResponse getStatus(ServerClientStatus& outStatus);
    ServerClientResponse getSnapshot(std::vector<RenderParticle>& outSnapshot,
                                     std::uint32_t maxPoints = 4096u,
                                     std::size_t* outSourceSize = nullptr);

private:
    typedef std::intptr_t SocketHandle;
    /// Description: Executes the trim operation.
    static std::string trim(const std::string& value);
    /// Description: Executes the readLine operation.
    bool readLine(std::string& outLine);
    SocketHandle _socket;
    int _socketTimeoutMs;
    bool _networkInitialized;
    std::string _recvBuffer;
    std::string _authToken;
};
#endif // GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERCLIENT_HPP_
