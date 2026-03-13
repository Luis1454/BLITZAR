#ifndef GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERCLIENT_HPP_
#define GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERCLIENT_HPP_

#include "protocol/ServerJsonCodec.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct ServerClientResponse {
    bool ok = false;
    std::string raw;
    std::string error;
};

struct ServerClientStatus {
    std::uint64_t steps = 0;
    float dt = 0.0f;
    bool paused = false;
    bool faulted = false;
    std::uint64_t faultStep = 0;
    std::string faultReason;
    bool sphEnabled = false;
    float serverFps = 0.0f;
    float substepTargetDt = 0.0f;
    float substepDt = 0.0f;
    std::uint32_t substeps = 0;
    std::uint32_t maxSubsteps = 0;
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
};

class ServerClient {
    public:
        ServerClient();
        ~ServerClient();

        bool connect(const std::string &host, std::uint16_t port);
        void setSocketTimeoutMs(int timeoutMs);
        int socketTimeoutMs() const;
        void setAuthToken(std::string token);
        void disconnect();
        bool isConnected() const;

        ServerClientResponse sendJson(const std::string &jsonLine);
        ServerClientResponse sendCommand(const std::string &cmd, const std::string &fieldsJson = "");
        ServerClientResponse getStatus(ServerClientStatus &outStatus);
        ServerClientResponse getSnapshot(std::vector<RenderParticle> &outSnapshot, std::uint32_t maxPoints = 4096u);

    private:
        typedef std::intptr_t SocketHandle;
        static std::string trim(const std::string &value);
        bool readLine(std::string &outLine);

        SocketHandle _socket;
        int _socketTimeoutMs;
        bool _networkInitialized;
        std::string _recvBuffer;
        std::string _authToken;
};



#endif // GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERCLIENT_HPP_
