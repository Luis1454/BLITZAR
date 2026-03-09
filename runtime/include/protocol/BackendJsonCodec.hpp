#pragma once

#include "backend/SimulationBackend.hpp"
#include "config/TextParse.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace grav_protocol {

struct BackendCommandRequest {
    std::string cmd;
    std::string token;
};

struct BackendResponseEnvelope {
    bool ok = false;
    std::string cmd;
    std::string error;
};

struct BackendStatusPayload {
    BackendResponseEnvelope envelope;
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

struct BackendSnapshotPayload {
    BackendResponseEnvelope envelope;
    bool hasSnapshot = false;
    std::vector<RenderParticle> particles;
};

class BackendJsonCodec {
    public:
        static std::string escapeString(std::string_view value);

        static std::string makeCommandRequest(
            const BackendCommandRequest &request,
            std::string_view fieldsJson = {});
        static bool parseCommandRequest(std::string_view raw, BackendCommandRequest &out,
                                        std::string &error);

        static std::string makeOkResponse(std::string_view cmd);
        static std::string makeErrorResponse(std::string_view cmd, std::string_view message);
        static std::string makeStatusResponse(const SimulationStats &stats);
        static std::string makeSnapshotResponse(bool hasSnapshot,
                                                const std::vector<RenderParticle> &snapshot);

        static bool parseResponseEnvelope(std::string_view raw, BackendResponseEnvelope &out,
                                          std::string &error);

        static bool parseStatusResponse(std::string_view raw, BackendStatusPayload &out,
                                        std::string &error);

        static bool parseSnapshotResponse(std::string_view raw, BackendSnapshotPayload &out,
                                          std::string &error);

        static bool readString(std::string_view raw, std::string_view key, std::string &out);
        static bool readBool(std::string_view raw, std::string_view key, bool &out);

        template <typename NumberType>
        static bool readNumber(std::string_view raw, std::string_view key, NumberType &out);

    private:
        static std::string trim(std::string_view value);
        static std::string toLower(std::string value);
        static bool findValueStart(std::string_view raw, std::string_view key, std::size_t &start);
        static bool readToken(std::string_view raw, std::string_view key, std::string &out);
};

} // namespace grav_protocol

