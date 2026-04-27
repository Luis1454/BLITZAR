/*
 * @file runtime/include/protocol/ServerClient.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERCLIENT_HPP_
#define GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERCLIENT_HPP_
#include "protocol/ServerJsonCodec.hpp"
#include <cstdint>
#include <string>
#include <vector>

/*
 * @brief Defines the server client response type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct ServerClientResponse {
    bool ok = false;
    std::string raw;
    std::string error;
};

/*
 * @brief Defines the server client status type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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

/*
 * @brief Defines the server client type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class ServerClient {
public:
    ServerClient();
    /*
     * @brief Documents the ~server client operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    ~ServerClient();
    /*
     * @brief Documents the connect operation contract.
     * @param host Input value used by this contract.
     * @param port Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool connect(const std::string& host, std::uint16_t port);
    /*
     * @brief Documents the set socket timeout ms operation contract.
     * @param timeoutMs Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setSocketTimeoutMs(int timeoutMs);
    /*
     * @brief Documents the socket timeout ms operation contract.
     * @param None This contract does not take explicit parameters.
     * @return int value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    int socketTimeoutMs() const;
    /*
     * @brief Documents the set auth token operation contract.
     * @param token Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setAuthToken(std::string token);
    /*
     * @brief Documents the disconnect operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void disconnect();
    /*
     * @brief Documents the is connected operation contract.
     * @param None This contract does not take explicit parameters.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool isConnected() const;
    /*
     * @brief Documents the send json operation contract.
     * @param jsonLine Input value used by this contract.
     * @return ServerClientResponse value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    ServerClientResponse sendJson(const std::string& jsonLine);
    /*
     * @brief Documents the send command operation contract.
     * @param cmd Input value used by this contract.
     * @param fieldsJson Input value used by this contract.
     * @return ServerClientResponse value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    ServerClientResponse sendCommand(const std::string& cmd, const std::string& fieldsJson = "");
    /*
     * @brief Documents the get status operation contract.
     * @param outStatus Input value used by this contract.
     * @return ServerClientResponse value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    ServerClientResponse getStatus(ServerClientStatus& outStatus);
    /*
     * @brief Documents the get snapshot operation contract.
     * @param outSnapshot Input value used by this contract.
     * @param maxPoints Input value used by this contract.
     * @param outSourceSize Input value used by this contract.
     * @return ServerClientResponse value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    ServerClientResponse getSnapshot(std::vector<RenderParticle>& outSnapshot,
                                     std::uint32_t maxPoints = 4096u,
                                     std::size_t* outSourceSize = nullptr);

private:
    typedef std::intptr_t SocketHandle;
    /*
     * @brief Documents the trim operation contract.
     * @param value Input value used by this contract.
     * @return std::string value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static std::string trim(const std::string& value);
    /*
     * @brief Documents the read line operation contract.
     * @param outLine Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool readLine(std::string& outLine);
    SocketHandle _socket;
    int _socketTimeoutMs;
    bool _networkInitialized;
    std::string _recvBuffer;
    std::string _authToken;
};
#endif // GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERCLIENT_HPP_
