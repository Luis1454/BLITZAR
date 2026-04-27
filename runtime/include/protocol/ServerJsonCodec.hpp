// File: runtime/include/protocol/ServerJsonCodec.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERJSONCODEC_HPP_
#define GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERJSONCODEC_HPP_
#include "config/TextParse.hpp"
#include "types/SimulationTypes.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace grav_protocol {
/// Description: Defines the ServerCommandRequest data or behavior contract.
struct ServerCommandRequest {
    std::string cmd;
    std::string token;
};

/// Description: Defines the ServerResponseEnvelope data or behavior contract.
struct ServerResponseEnvelope {
    bool ok = false;
    std::string cmd;
    std::string error;
};

/// Description: Defines the ServerStatusPayload data or behavior contract.
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

/// Description: Defines the ServerSnapshotPayload data or behavior contract.
struct ServerSnapshotPayload {
    ServerResponseEnvelope envelope;
    bool hasSnapshot = false;
    std::size_t sourceSize = 0u;
    std::vector<RenderParticle> particles;
};

/// Description: Defines the ServerJsonCodec data or behavior contract.
class ServerJsonCodec {
public:
    /// Description: Describes the escape string operation contract.
    static std::string escapeString(std::string_view value);
    /// Description: Describes the make command request operation contract.
    static std::string makeCommandRequest(const ServerCommandRequest& request,
                                          std::string_view fieldsJson = {});
    /// Description: Describes the parse command request operation contract.
    static bool parseCommandRequest(std::string_view raw, ServerCommandRequest& out,
                                    std::string& error);
    /// Description: Describes the make ok response operation contract.
    static std::string makeOkResponse(std::string_view cmd);
    /// Description: Describes the make error response operation contract.
    static std::string makeErrorResponse(std::string_view cmd, std::string_view message);
    /// Description: Describes the make status response operation contract.
    static std::string makeStatusResponse(const SimulationStats& stats);
    /// Description: Describes the make snapshot response operation contract.
    static std::string makeSnapshotResponse(bool hasSnapshot,
                                            const std::vector<RenderParticle>& snapshot,
                                            std::size_t sourceSize = 0u);
    /// Description: Describes the parse response envelope operation contract.
    static bool parseResponseEnvelope(std::string_view raw, ServerResponseEnvelope& out,
                                      std::string& error);
    /// Description: Describes the parse status response operation contract.
    static bool parseStatusResponse(std::string_view raw, ServerStatusPayload& out,
                                    std::string& error);
    /// Description: Describes the parse snapshot response operation contract.
    static bool parseSnapshotResponse(std::string_view raw, ServerSnapshotPayload& out,
                                      std::string& error);
    /// Description: Describes the read string operation contract.
    static bool readString(std::string_view raw, std::string_view key, std::string& out);
    /// Description: Describes the read bool operation contract.
    static bool readBool(std::string_view raw, std::string_view key, bool& out);
    template <typename NumberType>
    /// Description: Describes the read number operation contract.
    static bool readNumber(std::string_view raw, std::string_view key, NumberType& out);

private:
    /// Description: Describes the trim operation contract.
    static std::string trim(std::string_view value);
    /// Description: Describes the to lower operation contract.
    static std::string toLower(std::string value);
    /// Description: Describes the find value start operation contract.
    static bool findValueStart(std::string_view raw, std::string_view key, std::size_t& start);
    /// Description: Describes the read token operation contract.
    static bool readToken(std::string_view raw, std::string_view key, std::string& out);
};
} // namespace grav_protocol
#endif // GRAVITY_RUNTIME_INCLUDE_PROTOCOL_SERVERJSONCODEC_HPP_
