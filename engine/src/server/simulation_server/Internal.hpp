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
std::string toLower(std::string value);
std::string trim(std::string value);
std::string normalizeSnapshotFormat(std::string format);
ParticleSystem::SolverMode solverModeFromCanonicalName(std::string_view name);
ParticleSystem::IntegratorMode integratorModeFromCanonicalName(std::string_view name);
std::string_view solverLabel(ParticleSystem::SolverMode mode);
constexpr std::size_t kMaxImportedParticles = 2'000'000;
constexpr std::uint32_t kPairwiseRealtimeParticleLimit = 20'000u;
std::uint32_t resolvePublishedSnapshotCap(std::uint32_t drawCap);
bool readRawBytes(std::istream& in, std::byte* data, std::size_t size);
bool writeRawBytes(std::ostream& out, const std::byte* data, std::size_t size);
std::string readEnvironment(std::string_view key);
bool isValidImportedParticleCount(std::size_t count);
bool isAutoSolverFallbackEnabled();
bool shouldForceCudaFailureOnceForTesting(std::string_view solver);
bool coerceConfigSolverIntegratorCompatibility(std::string& solver, std::string& integrator,
                                               std::string_view source);
float autoTargetSubstepDt(std::string_view solver, bool eulerIntegrator, bool sphEnabled,
                          std::size_t liveParticleCount);
OctreeOpeningCriterion openingCriterionFromCanonicalName(std::string_view name);
float clampThetaBound(float value);
float computeOctreeDistributionScore(const std::vector<Particle>& particles);
float profileThetaBias(std::string_view performanceProfile);
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
std::string guessFormatFromPath(const std::string& path);
bool readBeU32(std::istream& in, std::uint32_t& outValue);
bool readBeI32(std::istream& in, std::int32_t& outValue);
bool readBeF32(std::istream& in, float& outValue);
void writeBeU32(std::ostream& out, std::uint32_t value);
void writeBeI32(std::ostream& out, std::int32_t value);
void writeBeF32(std::ostream& out, float value);
void consumeOptionalLineBreak(std::istream& in);
struct BinarySnapshotHeader {
    char magic[8];
    std::uint32_t version;
    std::uint32_t count;
};
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
struct AsyncExportJob final {
    std::string outputPath;
    std::string format;
    std::vector<Particle> particles;
    std::string solverModeLabel;
    std::string integratorModeLabel;
    std::uint64_t step = 0u;
};
struct SimulationServer::ExportQueueState final {
    std::mutex mutex;
    std::condition_variable condition;
    std::deque<AsyncExportJob> jobs;
    bool stopRequested = false;
    std::thread worker;
};
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
struct CheckpointSaveResult final {
    std::mutex mutex;
    std::condition_variable condition;
    bool completed = false;
    bool ok = false;
    std::string error;
};
struct PendingCheckpointSaveRequest final {
    std::string outputPath;
    std::shared_ptr<CheckpointSaveResult> result;
};
struct SimulationCheckpointQueueState final {
    std::mutex mutex;
    std::deque<PendingCheckpointSaveRequest> saveRequests;
};
bool readLeU64(std::istream& in, std::uint64_t& outValue);
void writeLeU64(std::ostream& out, std::uint64_t value);
void writeLeU32(std::ostream& out, std::uint32_t value);
bool readLeU32(std::istream& in, std::uint32_t& outValue);
void writeLeF32(std::ostream& out, float value);
bool readLeF32(std::istream& in, float& outValue);
bool writeSizedString(std::ostream& out, const std::string& value);
bool readSizedString(std::istream& in, std::string& outValue, std::size_t maxLength);
bool isSupportedCheckpointString(std::string_view solver, std::string_view integrator,
                                 std::string_view profile, std::string_view criterion);
bool writeCheckpointFile(const std::string& outputPath, const SimulationCheckpointState& state,
                         std::string* outError);
bool readCheckpointFile(const std::string& inputPath, SimulationCheckpointState& outState,
                        std::string* outError);
bool writeExportSnapshotFile(const AsyncExportJob& job);
bool parseBinarySnapshot(const std::string& inputPath, std::vector<Particle>& outParticles);
bool parseXyzSnapshot(const std::string& inputPath, std::vector<Particle>& outParticles);
bool parseVtkSnapshot(const std::string& inputPath, std::vector<Particle>& outParticles);
bool parseSnapshotByFormat(std::string_view format, const std::string& inputPath,
                           std::vector<Particle>& outParticles);
bool parseSnapshotWithFallback(const std::string& inputPath, std::vector<Particle>& outParticles);
bool buildGeneratedState(std::vector<Particle>& outParticles, std::uint32_t particleCount,
                         const InitialStateConfig& config);
void atomicAddFloat(std::atomic<float>& atom, float val);
#endif // GRAVITY_ENGINE_SRC_SERVER_SIMULATION_SERVER_INTERNAL_HPP_
