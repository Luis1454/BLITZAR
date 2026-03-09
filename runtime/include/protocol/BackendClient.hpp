#pragma once

#include "protocol/BackendJsonCodec.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct BackendClientResponse {
    bool ok = false;
    std::string raw;
    std::string error;
};

struct BackendClientStatus {
    std::uint64_t steps = 0;
    float dt = 0.0f;
    bool paused = false;
    bool faulted = false;
    std::uint64_t faultStep = 0;
    std::string faultReason;
    bool sphEnabled = false;
    float backendFps = 0.0f;
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

class BackendClient {
    public:
        BackendClient();
        ~BackendClient();

        bool connect(const std::string &host, std::uint16_t port);
        void setSocketTimeoutMs(int timeoutMs);
        int socketTimeoutMs() const;
        void setAuthToken(std::string token);
        void disconnect();
        bool isConnected() const;

        BackendClientResponse sendJson(const std::string &jsonLine);
        BackendClientResponse sendCommand(const std::string &cmd, const std::string &fieldsJson = "");
        BackendClientResponse getStatus(BackendClientStatus &outStatus);
        BackendClientResponse getSnapshot(std::vector<RenderParticle> &outSnapshot, std::uint32_t maxPoints = 4096u);

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


