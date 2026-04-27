/*
 * @file runtime/include/protocol/ServerJsonCodec.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERJSONCODEC_HPP_
#define GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERJSONCODEC_HPP_
#include "config/TextParse.hpp"
#include "types/SimulationTypes.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace grav_protocol {
struct ServerCommandRequest {
    std::string cmd;
    std::string token;
};

struct ServerResponseEnvelope {
    bool ok = false;
    std::string cmd;
    std::string error;
};

struct ServerStatusPayload {
    ServerResponseEnvelope envelope;
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

struct ServerSnapshotPayload {
    ServerResponseEnvelope envelope;
    bool hasSnapshot = false;
    std::size_t sourceSize = 0u;
    std::vector<RenderParticle> particles;
};

class ServerJsonCodec {
public:
    static std::string escapeString(std::string_view value);
    static std::string makeCommandRequest(const ServerCommandRequest& request,
                                          std::string_view fieldsJson = {});
    static bool parseCommandRequest(std::string_view raw, ServerCommandRequest& out,
                                    std::string& error);
    static std::string makeOkResponse(std::string_view cmd);
    static std::string makeErrorResponse(std::string_view cmd, std::string_view message);
    static std::string makeStatusResponse(const SimulationStats& stats);
    static std::string makeSnapshotResponse(bool hasSnapshot,
                                            const std::vector<RenderParticle>& snapshot,
                                            std::size_t sourceSize = 0u);
    static bool parseResponseEnvelope(std::string_view raw, ServerResponseEnvelope& out,
                                      std::string& error);
    static bool parseStatusResponse(std::string_view raw, ServerStatusPayload& out,
                                    std::string& error);
    static bool parseSnapshotResponse(std::string_view raw, ServerSnapshotPayload& out,
                                      std::string& error);
    static bool readString(std::string_view raw, std::string_view key, std::string& out);
    static bool readBool(std::string_view raw, std::string_view key, bool& out);
    template <typename NumberType>
    static bool readNumber(std::string_view raw, std::string_view key, NumberType& out);

private:
    static std::string trim(std::string_view value);
    static std::string toLower(std::string value);
    static bool findValueStart(std::string_view raw, std::string_view key, std::size_t& start);
    static bool readToken(std::string_view raw, std::string_view key, std::string& out);
};
} // namespace grav_protocol
#endif // GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERJSONCODEC_HPP_
