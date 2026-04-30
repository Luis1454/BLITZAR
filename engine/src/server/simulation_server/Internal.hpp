/*
 * @file engine/src/server/simulation_server/Internal.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Source artifact for the BLITZAR simulation project.
 */

#ifndef BLITZAR_ENGINE_SRC_SERVER_SIMULATION_SERVER_INTERNAL_HPP_
#define BLITZAR_ENGINE_SRC_SERVER_SIMULATION_SERVER_INTERNAL_HPP_
#include "config/EnvUtils.hpp"
#include "config/SimulationModes.hpp"
#include "config/SimulationPerformanceProfile.hpp"
#include "config/SimulationProfile.hpp"
#include "platform/PlatformPaths.hpp"
#include "protocol/ServerProtocol.hpp"
#include "server/SimulationInitConfig.hpp"
#include "server/SimulationServer.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cuda_runtime.h>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>
/*
 * @brief Documents the to lower operation contract.
 * @param value Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string toLower(std::string value);
/*
 * @brief Documents the trim operation contract.
 * @param value Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string trim(std::string value);
/*
 * @brief Documents the normalize snapshot format operation contract.
 * @param format Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string normalizeSnapshotFormat(std::string format);
/*
 * @brief Documents the solver mode from canonical name operation contract.
 * @param name Input value used by this contract.
 * @return ParticleSystem::SolverMode value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ParticleSystem::SolverMode solverModeFromCanonicalName(std::string_view name);
/*
 * @brief Documents the integrator mode from canonical name operation contract.
 * @param name Input value used by this contract.
 * @return ParticleSystem::IntegratorMode value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ParticleSystem::IntegratorMode integratorModeFromCanonicalName(std::string_view name);
/*
 * @brief Documents the solver label operation contract.
 * @param mode Input value used by this contract.
 * @return std::string_view value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string_view solverLabel(ParticleSystem::SolverMode mode);
constexpr std::size_t kMaxImportedParticles = 2'000'000;
constexpr std::uint32_t kPairwiseRealtimeParticleLimit = 20'000u;
/*
 * @brief Documents the resolve published snapshot cap operation contract.
 * @param drawCap Input value used by this contract.
 * @return std::uint32_t value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::uint32_t resolvePublishedSnapshotCap(std::uint32_t drawCap);
/*
 * @brief Documents the read raw bytes operation contract.
 * @param in Input value used by this contract.
 * @param data Input value used by this contract.
 * @param size Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool readRawBytes(std::istream& in, std::byte* data, std::size_t size);
/*
 * @brief Documents the write raw bytes operation contract.
 * @param out Input value used by this contract.
 * @param data Input value used by this contract.
 * @param size Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool writeRawBytes(std::ostream& out, const std::byte* data, std::size_t size);
/*
 * @brief Documents the read environment operation contract.
 * @param key Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string readEnvironment(std::string_view key);
/*
 * @brief Documents the is valid imported particle count operation contract.
 * @param count Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool isValidImportedParticleCount(std::size_t count);
/*
 * @brief Documents the is auto solver fallback enabled operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool isAutoSolverFallbackEnabled();
/*
 * @brief Documents the should force cuda failure once for testing operation contract.
 * @param solver Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool shouldForceCudaFailureOnceForTesting(std::string_view solver);
/*
 * @brief Documents the coerce config solver integrator compatibility operation contract.
 * @param solver Input value used by this contract.
 * @param integrator Input value used by this contract.
 * @param source Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool coerceConfigSolverIntegratorCompatibility(std::string& solver, std::string& integrator,
                                               std::string_view source);
/*
 * @brief Documents the auto target substep dt operation contract.
 * @param solver Input value used by this contract.
 * @param eulerIntegrator Input value used by this contract.
 * @param sphEnabled Input value used by this contract.
 * @param liveParticleCount Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
float autoTargetSubstepDt(std::string_view solver, bool eulerIntegrator, bool sphEnabled,
                          std::size_t liveParticleCount);
/*
 * @brief Documents the opening criterion from canonical name operation contract.
 * @param name Input value used by this contract.
 * @return OctreeOpeningCriterion value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
OctreeOpeningCriterion openingCriterionFromCanonicalName(std::string_view name);
/*
 * @brief Documents the clamp theta bound operation contract.
 * @param value Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
float clampThetaBound(float value);
/*
 * @brief Documents the compute octree distribution score operation contract.
 * @param particles Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
float computeOctreeDistributionScore(const std::vector<Particle>& particles);
/*
 * @brief Documents the profile theta bias operation contract.
 * @param performanceProfile Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
float profileThetaBias(std::string_view performanceProfile);
/*
 * @brief Documents the particle theta bias operation contract.
 * @param particleCount Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
float particleThetaBias(std::size_t particleCount);
/*
 * @brief Documents the resolve octree theta operation contract.
 * @param configuredTheta Input value used by this contract.
 * @param autoTune Input value used by this contract.
 * @param autoMin Input value used by this contract.
 * @param autoMax Input value used by this contract.
 * @param performanceProfile Input value used by this contract.
 * @param particles Input value used by this contract.
 * @param distributionScore Input value used by this contract.
 * @return float value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
float resolveOctreeTheta(float configuredTheta, bool autoTune, float autoMin, float autoMax,
                         std::string_view performanceProfile,
                         const std::vector<Particle>& particles, float distributionScore);
/*
 * @brief Documents the log effective execution modes operation contract.
 * @param solver Input value used by this contract.
 * @param integrator Input value used by this contract.
 * @param performanceProfile Input value used by this contract.
 * @param openingCriterion Input value used by this contract.
 * @param theta Input value used by this contract.
 * @param effectiveTheta Input value used by this contract.
 * @param thetaAutoTune Input value used by this contract.
 * @param thetaAutoMin Input value used by this contract.
 * @param thetaAutoMax Input value used by this contract.
 * @param octreeDistributionScore Input value used by this contract.
 * @param softening Input value used by this contract.
 * @param physicsMaxAcceleration Input value used by this contract.
 * @param physicsMinSoftening Input value used by this contract.
 * @param physicsMinDistance2 Input value used by this contract.
 * @param physicsMinTheta Input value used by this contract.
 * @param sphEnabled Input value used by this contract.
 * @param configuredSubstepTargetDt Input value used by this contract.
 * @param configuredMaxSubsteps Input value used by this contract.
 * @param snapshotPublishPeriodMs Input value used by this contract.
 * @param serverFps Input value used by this contract.
 * @param energyDriftPct Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void logEffectiveExecutionModes(
    std::string_view solver, std::string_view integrator, std::string_view performanceProfile,
    std::string_view openingCriterion, float theta, float effectiveTheta, bool thetaAutoTune,
    float thetaAutoMin, float thetaAutoMax, float octreeDistributionScore, float softening,
    float physicsMaxAcceleration, float physicsMinSoftening, float physicsMinDistance2,
    float physicsMinTheta, bool sphEnabled, float configuredSubstepTargetDt,
    std::uint32_t configuredMaxSubsteps, std::uint32_t snapshotPublishPeriodMs, float serverFps,
    float energyDriftPct);
/*
 * @brief Documents the default export path operation contract.
 * @param directory Input value used by this contract.
 * @param format Input value used by this contract.
 * @param step Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string defaultExportPath(const std::string& directory, const std::string& format,
                              std::uint64_t step);
/*
 * @brief Documents the guess format from path operation contract.
 * @param path Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string guessFormatFromPath(const std::string& path);
/*
 * @brief Documents the read be u32 operation contract.
 * @param in Input value used by this contract.
 * @param outValue Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool readBeU32(std::istream& in, std::uint32_t& outValue);
/*
 * @brief Documents the read be i32 operation contract.
 * @param in Input value used by this contract.
 * @param outValue Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool readBeI32(std::istream& in, std::int32_t& outValue);
/*
 * @brief Documents the read be f32 operation contract.
 * @param in Input value used by this contract.
 * @param outValue Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool readBeF32(std::istream& in, float& outValue);
/*
 * @brief Documents the write be u32 operation contract.
 * @param out Input value used by this contract.
 * @param value Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void writeBeU32(std::ostream& out, std::uint32_t value);
/*
 * @brief Documents the write be i32 operation contract.
 * @param out Input value used by this contract.
 * @param value Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void writeBeI32(std::ostream& out, std::int32_t value);
/*
 * @brief Documents the write be f32 operation contract.
 * @param out Input value used by this contract.
 * @param value Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void writeBeF32(std::ostream& out, float value);
/*
 * @brief Documents the consume optional line break operation contract.
 * @param in Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void consumeOptionalLineBreak(std::istream& in);

/*
 * @brief Defines the binary snapshot header type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct BinarySnapshotHeader {
    char magic[8];
    std::uint32_t version;
    std::uint32_t count;
};

/*
 * @brief Defines the binary snapshot particle type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct BinarySnapshotParticle {
    float px;
    float py;
    float pz;
    float vx;
    float vy;
    float vz;
    float mass;
    float temperature;
};

constexpr char kBinarySnapshotMagic[8] = {'N', 'B', 'S', 'I', 'M', 'B', 'I', 'N'};
constexpr std::uint32_t kBinarySnapshotVersion = 1u;
constexpr char kCheckpointMagic[8] = {'B', 'L', 'T', 'Z', 'C', 'H', 'K', '1'};
constexpr std::uint32_t kCheckpointVersion = 1u;
constexpr std::uint32_t kCheckpointFlagPaused = 1u << 0;
constexpr std::uint32_t kCheckpointFlagHasEnergyBaseline = 1u << 1;
constexpr std::uint32_t kCheckpointFlagSphEnabled = 1u << 2;
constexpr std::uint32_t kCheckpointFlagThetaAutoTune = 1u << 3;
constexpr std::uint32_t kCheckpointFlagGpuTelemetryEnabled = 1u << 4;

/*
 * @brief Defines the async export job type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct AsyncExportJob final {
    std::string outputPath;
    std::string format;
    std::vector<Particle> particles;
    std::string solverModeLabel;
    std::string integratorModeLabel;
    std::uint64_t step = 0u;
};

/*
 * @brief Defines the simulation server type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationServer::ExportQueueState final {
    std::mutex mutex;
    std::condition_variable condition;
    std::deque<AsyncExportJob> jobs;
    bool stopRequested = false;
    std::thread worker;
};

/*
 * @brief Defines the simulation checkpoint state type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationCheckpointState final {
    SimulationConfig config;
    std::vector<Particle> particles;
    std::uint64_t steps = 0u;
    float totalTime = 0.0f;
    bool paused = false;
    bool hasEnergyBaseline = false;
    float energyBaseline = 0.0f;
    bool gpuTelemetryEnabled = false;
};

/*
 * @brief Defines the checkpoint save result type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct CheckpointSaveResult final {
    std::mutex mutex;
    std::condition_variable condition;
    bool completed = false;
    bool ok = false;
    std::string error;
};

/*
 * @brief Defines the pending checkpoint save request type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct PendingCheckpointSaveRequest final {
    std::string outputPath;
    std::shared_ptr<CheckpointSaveResult> result;
};

/*
 * @brief Defines the simulation checkpoint queue state type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationCheckpointQueueState final {
    std::mutex mutex;
    std::deque<PendingCheckpointSaveRequest> saveRequests;
};

/*
 * @brief Documents the read le u64 operation contract.
 * @param in Input value used by this contract.
 * @param outValue Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool readLeU64(std::istream& in, std::uint64_t& outValue);
/*
 * @brief Documents the write le u64 operation contract.
 * @param out Input value used by this contract.
 * @param value Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void writeLeU64(std::ostream& out, std::uint64_t value);
/*
 * @brief Documents the write le u32 operation contract.
 * @param out Input value used by this contract.
 * @param value Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void writeLeU32(std::ostream& out, std::uint32_t value);
/*
 * @brief Documents the read le u32 operation contract.
 * @param in Input value used by this contract.
 * @param outValue Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool readLeU32(std::istream& in, std::uint32_t& outValue);
/*
 * @brief Documents the write le f32 operation contract.
 * @param out Input value used by this contract.
 * @param value Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void writeLeF32(std::ostream& out, float value);
/*
 * @brief Documents the read le f32 operation contract.
 * @param in Input value used by this contract.
 * @param outValue Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool readLeF32(std::istream& in, float& outValue);
/*
 * @brief Documents the write sized string operation contract.
 * @param out Input value used by this contract.
 * @param value Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool writeSizedString(std::ostream& out, const std::string& value);
/*
 * @brief Documents the read sized string operation contract.
 * @param in Input value used by this contract.
 * @param outValue Input value used by this contract.
 * @param maxLength Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool readSizedString(std::istream& in, std::string& outValue, std::size_t maxLength);
/*
 * @brief Documents the is supported checkpoint string operation contract.
 * @param solver Input value used by this contract.
 * @param integrator Input value used by this contract.
 * @param profile Input value used by this contract.
 * @param criterion Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool isSupportedCheckpointString(std::string_view solver, std::string_view integrator,
                                 std::string_view profile, std::string_view criterion);
/*
 * @brief Documents the write checkpoint file operation contract.
 * @param outputPath Input value used by this contract.
 * @param state Input value used by this contract.
 * @param outError Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool writeCheckpointFile(const std::string& outputPath, const SimulationCheckpointState& state,
                         std::string* outError);
/*
 * @brief Documents the read checkpoint file operation contract.
 * @param inputPath Input value used by this contract.
 * @param outState Input value used by this contract.
 * @param outError Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool readCheckpointFile(const std::string& inputPath, SimulationCheckpointState& outState,
                        std::string* outError);
/*
 * @brief Documents the write export snapshot file operation contract.
 * @param job Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool writeExportSnapshotFile(const AsyncExportJob& job);
/*
 * @brief Documents the parse binary snapshot operation contract.
 * @param inputPath Input value used by this contract.
 * @param outParticles Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool parseBinarySnapshot(const std::string& inputPath, std::vector<Particle>& outParticles);
/*
 * @brief Documents the parse xyz snapshot operation contract.
 * @param inputPath Input value used by this contract.
 * @param outParticles Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool parseXyzSnapshot(const std::string& inputPath, std::vector<Particle>& outParticles);
/*
 * @brief Documents the parse vtk snapshot operation contract.
 * @param inputPath Input value used by this contract.
 * @param outParticles Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool parseVtkSnapshot(const std::string& inputPath, std::vector<Particle>& outParticles);
/*
 * @brief Documents the parse snapshot by format operation contract.
 * @param format Input value used by this contract.
 * @param inputPath Input value used by this contract.
 * @param outParticles Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool parseSnapshotByFormat(std::string_view format, const std::string& inputPath,
                           std::vector<Particle>& outParticles);
/*
 * @brief Documents the parse snapshot with fallback operation contract.
 * @param inputPath Input value used by this contract.
 * @param outParticles Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool parseSnapshotWithFallback(const std::string& inputPath, std::vector<Particle>& outParticles);
/*
 * @brief Documents the build generated state operation contract.
 * @param outParticles Input value used by this contract.
 * @param particleCount Input value used by this contract.
 * @param config Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool buildGeneratedState(std::vector<Particle>& outParticles, std::uint32_t particleCount,
                         const InitialStateConfig& config);
/*
 * @brief Documents the atomic add float operation contract.
 * @param atom Input value used by this contract.
 * @param val Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void atomicAddFloat(std::atomic<float>& atom, float val);
#endif // BLITZAR_ENGINE_SRC_SERVER_SIMULATION_SERVER_INTERNAL_HPP_
