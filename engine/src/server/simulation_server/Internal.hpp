// File: engine/src/server/simulation_server/Internal.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_SRC_SERVER_SIMULATION_SERVER_INTERNAL_HPP_
#define GRAVITY_ENGINE_SRC_SERVER_SIMULATION_SERVER_INTERNAL_HPP_
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
/// Description: Executes the toLower operation.
std::string toLower(std::string value);
/// Description: Executes the trim operation.
std::string trim(std::string value);
/// Description: Executes the normalizeSnapshotFormat operation.
std::string normalizeSnapshotFormat(std::string format);
/// Description: Executes the solverModeFromCanonicalName operation.
ParticleSystem::SolverMode solverModeFromCanonicalName(std::string_view name);
/// Description: Executes the integratorModeFromCanonicalName operation.
ParticleSystem::IntegratorMode integratorModeFromCanonicalName(std::string_view name);
/// Description: Executes the solverLabel operation.
std::string_view solverLabel(ParticleSystem::SolverMode mode);
constexpr std::size_t kMaxImportedParticles = 2'000'000;
constexpr std::uint32_t kPairwiseRealtimeParticleLimit = 20'000u;
/// Description: Executes the resolvePublishedSnapshotCap operation.
std::uint32_t resolvePublishedSnapshotCap(std::uint32_t drawCap);
/// Description: Executes the readRawBytes operation.
bool readRawBytes(std::istream& in, std::byte* data, std::size_t size);
/// Description: Executes the writeRawBytes operation.
bool writeRawBytes(std::ostream& out, const std::byte* data, std::size_t size);
/// Description: Executes the readEnvironment operation.
std::string readEnvironment(std::string_view key);
/// Description: Executes the isValidImportedParticleCount operation.
bool isValidImportedParticleCount(std::size_t count);
/// Description: Executes the isAutoSolverFallbackEnabled operation.
bool isAutoSolverFallbackEnabled();
/// Description: Executes the shouldForceCudaFailureOnceForTesting operation.
bool shouldForceCudaFailureOnceForTesting(std::string_view solver);
bool coerceConfigSolverIntegratorCompatibility(std::string& solver, std::string& integrator,
                                               std::string_view source);
float autoTargetSubstepDt(std::string_view solver, bool eulerIntegrator, bool sphEnabled,
                          std::size_t liveParticleCount);
/// Description: Executes the openingCriterionFromCanonicalName operation.
OctreeOpeningCriterion openingCriterionFromCanonicalName(std::string_view name);
/// Description: Executes the clampThetaBound operation.
float clampThetaBound(float value);
/// Description: Executes the computeOctreeDistributionScore operation.
float computeOctreeDistributionScore(const std::vector<Particle>& particles);
/// Description: Executes the profileThetaBias operation.
float profileThetaBias(std::string_view performanceProfile);
/// Description: Executes the particleThetaBias operation.
float particleThetaBias(std::size_t particleCount);
float resolveOctreeTheta(float configuredTheta, bool autoTune, float autoMin, float autoMax,
                         std::string_view performanceProfile,
                         const std::vector<Particle>& particles, float distributionScore);
void logEffectiveExecutionModes(
    std::string_view solver, std::string_view integrator, std::string_view performanceProfile,
    std::string_view openingCriterion, float theta, float effectiveTheta, bool thetaAutoTune,
    float thetaAutoMin, float thetaAutoMax, float octreeDistributionScore, float softening,
    float physicsMaxAcceleration, float physicsMinSoftening, float physicsMinDistance2,
    float physicsMinTheta, bool sphEnabled, float configuredSubstepTargetDt,
    std::uint32_t configuredMaxSubsteps, std::uint32_t snapshotPublishPeriodMs, float serverFps,
    float energyDriftPct);
std::string defaultExportPath(const std::string& directory, const std::string& format,
                              std::uint64_t step);
/// Description: Executes the guessFormatFromPath operation.
std::string guessFormatFromPath(const std::string& path);
/// Description: Executes the readBeU32 operation.
bool readBeU32(std::istream& in, std::uint32_t& outValue);
/// Description: Executes the readBeI32 operation.
bool readBeI32(std::istream& in, std::int32_t& outValue);
/// Description: Executes the readBeF32 operation.
bool readBeF32(std::istream& in, float& outValue);
/// Description: Executes the writeBeU32 operation.
void writeBeU32(std::ostream& out, std::uint32_t value);
/// Description: Executes the writeBeI32 operation.
void writeBeI32(std::ostream& out, std::int32_t value);
/// Description: Executes the writeBeF32 operation.
void writeBeF32(std::ostream& out, float value);
/// Description: Executes the consumeOptionalLineBreak operation.
void consumeOptionalLineBreak(std::istream& in);
/// Description: Defines the BinarySnapshotHeader data or behavior contract.
struct BinarySnapshotHeader {
    char magic[8];
    std::uint32_t version;
    std::uint32_t count;
};
/// Description: Defines the BinarySnapshotParticle data or behavior contract.
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
/// Description: Defines the AsyncExportJob data or behavior contract.
struct AsyncExportJob final {
    std::string outputPath;
    std::string format;
    std::vector<Particle> particles;
    std::string solverModeLabel;
    std::string integratorModeLabel;
    std::uint64_t step = 0u;
};
/// Description: Defines the SimulationServer data or behavior contract.
struct SimulationServer::ExportQueueState final {
    std::mutex mutex;
    std::condition_variable condition;
    std::deque<AsyncExportJob> jobs;
    bool stopRequested = false;
    std::thread worker;
};
/// Description: Defines the SimulationCheckpointState data or behavior contract.
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
/// Description: Defines the CheckpointSaveResult data or behavior contract.
struct CheckpointSaveResult final {
    std::mutex mutex;
    std::condition_variable condition;
    bool completed = false;
    bool ok = false;
    std::string error;
};
/// Description: Defines the PendingCheckpointSaveRequest data or behavior contract.
struct PendingCheckpointSaveRequest final {
    std::string outputPath;
    std::shared_ptr<CheckpointSaveResult> result;
};
/// Description: Defines the SimulationCheckpointQueueState data or behavior contract.
struct SimulationCheckpointQueueState final {
    std::mutex mutex;
    std::deque<PendingCheckpointSaveRequest> saveRequests;
};
/// Description: Executes the readLeU64 operation.
bool readLeU64(std::istream& in, std::uint64_t& outValue);
/// Description: Executes the writeLeU64 operation.
void writeLeU64(std::ostream& out, std::uint64_t value);
/// Description: Executes the writeLeU32 operation.
void writeLeU32(std::ostream& out, std::uint32_t value);
/// Description: Executes the readLeU32 operation.
bool readLeU32(std::istream& in, std::uint32_t& outValue);
/// Description: Executes the writeLeF32 operation.
void writeLeF32(std::ostream& out, float value);
/// Description: Executes the readLeF32 operation.
bool readLeF32(std::istream& in, float& outValue);
/// Description: Executes the writeSizedString operation.
bool writeSizedString(std::ostream& out, const std::string& value);
/// Description: Executes the readSizedString operation.
bool readSizedString(std::istream& in, std::string& outValue, std::size_t maxLength);
bool isSupportedCheckpointString(std::string_view solver, std::string_view integrator,
                                 std::string_view profile, std::string_view criterion);
bool writeCheckpointFile(const std::string& outputPath, const SimulationCheckpointState& state,
                         std::string* outError);
bool readCheckpointFile(const std::string& inputPath, SimulationCheckpointState& outState,
                        std::string* outError);
/// Description: Executes the writeExportSnapshotFile operation.
bool writeExportSnapshotFile(const AsyncExportJob& job);
/// Description: Executes the parseBinarySnapshot operation.
bool parseBinarySnapshot(const std::string& inputPath, std::vector<Particle>& outParticles);
/// Description: Executes the parseXyzSnapshot operation.
bool parseXyzSnapshot(const std::string& inputPath, std::vector<Particle>& outParticles);
/// Description: Executes the parseVtkSnapshot operation.
bool parseVtkSnapshot(const std::string& inputPath, std::vector<Particle>& outParticles);
bool parseSnapshotByFormat(std::string_view format, const std::string& inputPath,
                           std::vector<Particle>& outParticles);
/// Description: Executes the parseSnapshotWithFallback operation.
bool parseSnapshotWithFallback(const std::string& inputPath, std::vector<Particle>& outParticles);
bool buildGeneratedState(std::vector<Particle>& outParticles, std::uint32_t particleCount,
                         const InitialStateConfig& config);
/// Description: Executes the atomicAddFloat operation.
void atomicAddFloat(std::atomic<float>& atom, float val);
#endif // GRAVITY_ENGINE_SRC_SERVER_SIMULATION_SERVER_INTERNAL_HPP_
