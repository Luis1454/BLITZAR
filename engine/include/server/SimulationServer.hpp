/*
 * @file engine/include/server/SimulationServer.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Source artifact for the BLITZAR simulation project.
 */

#ifndef GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONSERVER_HPP_
#define GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONSERVER_HPP_
/*
 * Module: server
 * Responsibility: Own the authoritative simulation runtime and publish
 * snapshots and telemetry.
 */
#include "config/SimulationConfig.hpp"
#include "physics/ParticleSystem.hpp"
#include "types/SimulationTypes.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
/*
 * @brief Defines the simulation checkpoint state type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationCheckpointState;
/*
 * @brief Defines the simulation checkpoint queue state type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationCheckpointQueueState;

/*
 * @brief Defines the simulation server type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class SimulationServer {
public:
    /*
     * @brief Documents the simulation server operation contract.
     * @param particleCount Input value used by this contract.
     * @param initialDt Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    explicit SimulationServer(std::uint32_t particleCount, float initialDt);
    /*
     * @brief Documents the simulation server operation contract.
     * @param configPath Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    explicit SimulationServer(const std::string& configPath);
    /*
     * @brief Documents the ~simulation server operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    ~SimulationServer();
    /*
     * @brief Documents the start operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void start();
    /*
     * @brief Documents the stop operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void stop();
    /*
     * @brief Documents the set paused operation contract.
     * @param paused Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setPaused(bool paused);
    /*
     * @brief Documents the is paused operation contract.
     * @param None This contract does not take explicit parameters.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool isPaused() const;
    /*
     * @brief Documents the toggle paused operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void togglePaused();
    /*
     * @brief Documents the step once operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void stepOnce();
    /*
     * @brief Documents the set particle count operation contract.
     * @param particleCount Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setParticleCount(std::uint32_t particleCount);
    /*
     * @brief Documents the set dt operation contract.
     * @param dt Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setDt(float dt);
    /*
     * @brief Documents the scale dt operation contract.
     * @param factor Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void scaleDt(float factor);
    /*
     * @brief Documents the get dt operation contract.
     * @param None This contract does not take explicit parameters.
     * @return float value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    float getDt() const;
    /*
     * @brief Documents the request reset operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void requestReset();
    /*
     * @brief Documents the request recover operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void requestRecover();
    /*
     * @brief Documents the set solver mode operation contract.
     * @param mode Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setSolverMode(const std::string& mode);
    /*
     * @brief Documents the set integrator mode operation contract.
     * @param mode Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setIntegratorMode(const std::string& mode);
    /*
     * @brief Documents the set performance profile operation contract.
     * @param profile Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setPerformanceProfile(const std::string& profile);
    /*
     * @brief Documents the set octree parameters operation contract.
     * @param theta Input value used by this contract.
     * @param softening Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setOctreeParameters(float theta, float softening);
    /*
     * @brief Documents the set sph enabled operation contract.
     * @param enabled Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setSphEnabled(bool enabled);
    /*
     * @brief Documents the set sph parameters operation contract.
     * @param smoothingLength Input value used by this contract.
     * @param restDensity Input value used by this contract.
     * @param gasConstant Input value used by this contract.
     * @param viscosity Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                          float viscosity);
    /*
     * @brief Documents the set substep policy operation contract.
     * @param targetDt Input value used by this contract.
     * @param maxSubsteps Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps);
    /*
     * @brief Documents the set snapshot publish period ms operation contract.
     * @param periodMs Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setSnapshotPublishPeriodMs(std::uint32_t periodMs);
    /*
     * @brief Documents the set initial state config operation contract.
     * @param config Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setInitialStateConfig(const InitialStateConfig& config);
    /*
     * @brief Documents the set energy measurement config operation contract.
     * @param everySteps Input value used by this contract.
     * @param sampleLimit Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit);
    /*
     * @brief Documents the set gpu telemetry enabled operation contract.
     * @param enabled Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setGpuTelemetryEnabled(bool enabled);
    /*
     * @brief Documents the set export defaults operation contract.
     * @param directory Input value used by this contract.
     * @param format Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setExportDefaults(const std::string& directory, const std::string& format);
    /*
     * @brief Documents the set snapshot transfer cap operation contract.
     * @param maxPoints Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setSnapshotTransferCap(std::uint32_t maxPoints);
    /*
     * @brief Documents the set initial state file operation contract.
     * @param path Input value used by this contract.
     * @param format Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void setInitialStateFile(const std::string& path, const std::string& format);
    /*
     * @brief Documents the request export snapshot operation contract.
     * @param outputPath Input value used by this contract.
     * @param format Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void requestExportSnapshot(const std::string& outputPath, const std::string& format);
    /*
     * @brief Documents the save checkpoint operation contract.
     * @param outputPath Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool saveCheckpoint(const std::string& outputPath);
    /*
     * @brief Documents the load checkpoint operation contract.
     * @param inputPath Input value used by this contract.
     * @param outError Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool loadCheckpoint(const std::string& inputPath, std::string* outError = nullptr);
    /*
     * @brief Documents the try consume snapshot operation contract.
     * @param outSnapshot Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool tryConsumeSnapshot(std::vector<RenderParticle>& outSnapshot);
    /*
     * @brief Documents the copy latest snapshot operation contract.
     * @param outSnapshot Input value used by this contract.
     * @param maxPoints Input value used by this contract.
     * @param outSourceSize Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool copyLatestSnapshot(std::vector<RenderParticle>& outSnapshot, std::size_t maxPoints = 0,
                            std::size_t* outSourceSize = nullptr) const;
    /*
     * @brief Documents the get stats operation contract.
     * @param None This contract does not take explicit parameters.
     * @return SimulationStats value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    SimulationStats getStats() const;
    /*
     * @brief Documents the get runtime config operation contract.
     * @param None This contract does not take explicit parameters.
     * @return SimulationConfig value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    SimulationConfig getRuntimeConfig() const;

private:
    /*
     * @brief Defines the energy values type contract.
     * @param None This contract does not take explicit parameters.
     * @return Not applicable; this block documents a type contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    struct EnergyValues {
        float kinetic;
        float potential;
        float thermal;
        float radiated;
        float total;
        bool estimated;
    };

    /*
     * @brief Defines the pending export request type contract.
     * @param None This contract does not take explicit parameters.
     * @return Not applicable; this block documents a type contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    struct PendingExportRequest final {
        std::string outputPath;
        std::string format;
    };
    /*
     * @brief Defines the export queue state type contract.
     * @param None This contract does not take explicit parameters.
     * @return Not applicable; this block documents a type contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    struct ExportQueueState;
    /*
     * @brief Documents the loop operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void loop();
    /*
     * @brief Documents the publish snapshot operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void publishSnapshot();
    /*
     * @brief Documents the rebuild system operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void rebuildSystem();
    /*
     * @brief Documents the compute energy values operation contract.
     * @param None This contract does not take explicit parameters.
     * @return EnergyValues value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    EnergyValues computeEnergyValues();
    /*
     * @brief Documents the maybe update energy operation contract.
     * @param currentStep Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void maybeUpdateEnergy(std::uint64_t currentStep);
    /*
     * @brief Documents the clear gpu telemetry operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void clearGpuTelemetry();
    /*
     * @brief Documents the maybe sample gpu telemetry operation contract.
     * @param solverMode Input value used by this contract.
     * @param currentStep Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void maybeSampleGpuTelemetry(std::string_view solverMode, std::uint64_t currentStep);
    /*
     * @brief Documents the process pending export operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void processPendingExport();
    /*
     * @brief Documents the process pending checkpoint save operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void processPendingCheckpointSave();
    /*
     * @brief Documents the export current state operation contract.
     * @param outputPath Input value used by this contract.
     * @param format Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool exportCurrentState(const std::string& outputPath, const std::string& format);
    /*
     * @brief Documents the capture checkpoint to file operation contract.
     * @param outputPath Input value used by this contract.
     * @param outError Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool captureCheckpointToFile(const std::string& outputPath, std::string* outError);
    /*
     * @brief Documents the start export worker operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void startExportWorker();
    /*
     * @brief Documents the stop export worker operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void stopExportWorker();
    /*
     * @brief Documents the enqueue export write operation contract.
     * @param outputPath Input value used by this contract.
     * @param format Input value used by this contract.
     * @param particles Input value used by this contract.
     * @param solverModeLabel Input value used by this contract.
     * @param integratorModeLabel Input value used by this contract.
     * @param step Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void enqueueExportWrite(const std::string& outputPath, const std::string& format,
                            const std::vector<Particle>& particles,
                            const std::string& solverModeLabel,
                            const std::string& integratorModeLabel, std::uint64_t step);
    /*
     * @brief Documents the update export status operation contract.
     * @param state Input value used by this contract.
     * @param path Input value used by this contract.
     * @param message Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void updateExportStatus(const std::string& state, const std::string& path,
                            const std::string& message);
    /*
     * @brief Documents the load initial state operation contract.
     * @param outParticles Input value used by this contract.
     * @param inputPath Input value used by this contract.
     * @param format Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool loadInitialState(std::vector<Particle>& outParticles, const std::string& inputPath,
                          const std::string& format) const;
    /*
     * @brief Documents the clear published snapshot cache operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void clearPublishedSnapshotCache();
    std::atomic<bool> _running;
    std::atomic<bool> _paused;
    std::atomic<bool> _resetRequested;
    std::atomic<bool> _cudaContextDirty;
    std::atomic<std::uint32_t> _stepRequests;
    std::atomic<float> _dt;
    std::atomic<float> _totalTime;
    std::atomic<std::uint64_t> _steps;
    std::atomic<float> _serverFps;
    std::atomic<float> _kineticEnergy;
    std::atomic<float> _potentialEnergy;
    std::atomic<float> _thermalEnergy;
    std::atomic<float> _radiatedEnergy;
    std::atomic<float> _totalEnergy;
    std::atomic<float> _energyDriftPct;
    std::atomic<bool> _energyEstimated;
    std::atomic<std::uint32_t> _energyMeasureEverySteps;
    std::atomic<std::uint32_t> _energySampleLimit;
    std::atomic<bool> _gpuTelemetryEnabled;
    std::atomic<bool> _gpuTelemetryAvailable;
    std::atomic<float> _gpuKernelMs;
    std::atomic<float> _gpuCopyMs;
    std::atomic<std::uint64_t> _gpuVramUsedBytes;
    std::atomic<std::uint64_t> _gpuVramTotalBytes;
    std::atomic<std::uint32_t> _exportQueueDepth;
    std::atomic<bool> _exportActive;
    std::atomic<std::uint64_t> _exportCompletedCount;
    std::atomic<std::uint64_t> _exportFailedCount;
    std::atomic<float> _configuredSubstepTargetDt;
    std::atomic<std::uint32_t> _configuredMaxSubsteps;
    std::atomic<std::uint32_t> _snapshotPublishPeriodMs;
    std::atomic<std::uint32_t> _snapshotTransferCap;
    std::atomic<float> _lastAppliedSubstepTargetDt;
    std::atomic<float> _lastAppliedSubstepDt;
    std::atomic<std::uint32_t> _lastAppliedSubsteps;
    std::atomic<bool> _faulted;
    std::atomic<std::uint64_t> _faultStep;
    std::uint32_t _particleCount;
    std::string _solverMode;
    std::string _integratorMode;
    std::string _performanceProfile;
    float _octreeTheta;
    float _octreeSoftening;
    std::string _octreeOpeningCriterion;
    float _octreeEffectiveTheta;
    float _octreeThetaAutoMin;
    float _octreeThetaAutoMax;
    float _octreeDistributionScore;
    bool _octreeThetaAutoTune;
    bool _sphEnabled;
    float _sphSmoothingLength;
    float _sphRestDensity;
    float _sphGasConstant;
    float _sphViscosity;
    float _physicsMaxAcceleration;
    float _physicsMinSoftening;
    float _physicsMinDistance2;
    float _physicsMinTheta;
    float _sphMaxAcceleration;
    float _sphMaxSpeed;
    float _energyBaseline;
    bool _hasEnergyBaseline;
    std::string _exportDirectory;
    std::string _exportFormatDefault;
    std::string _initialStatePath;
    std::string _initialStateFormat;
    std::string _configPath;
    SimulationConfig _runtimeConfigMirror;
    InitialStateConfig _initialStateConfig;
    std::thread _thread;
    mutable std::mutex _snapshotMutex;
    mutable std::mutex _commandMutex;
    mutable std::mutex _faultMutex;
    mutable std::mutex _exportStatusMutex;
    std::deque<PendingExportRequest> _pendingExportRequests;
    std::vector<RenderParticle> _publishedSnapshot;
    std::vector<RenderParticle> _scratchSnapshot;
    std::string _faultReason;
    std::string _exportLastState;
    std::string _exportLastPath;
    std::string _exportLastMessage;
    std::unique_ptr<ParticleSystem> _system;
    std::unique_ptr<ExportQueueState> _exportQueueState;
    std::unique_ptr<SimulationCheckpointState> _activeCheckpointState;
    std::unique_ptr<SimulationCheckpointQueueState> _checkpointQueueState;
    static constexpr std::uint64_t kGpuTelemetrySampleStride = 8u;
};
#endif // GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONSERVER_HPP_
