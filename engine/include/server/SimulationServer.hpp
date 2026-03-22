#ifndef GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONSERVER_HPP_
#define GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONSERVER_HPP_

/*
 * Module: server
 * Responsibility: Own the authoritative simulation runtime and publish snapshots and telemetry.
 */

#include "config/SimulationConfig.hpp"
#include "physics/ParticleSystem.hpp"
#include "types/SimulationTypes.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/// Runs the authoritative simulation loop and exposes deterministic control operations.
class SimulationServer {
    public:
        /// Builds a server from explicit particle-count and timestep defaults.
        explicit SimulationServer(std::uint32_t particleCount, float initialDt);
        /// Builds a server from a persisted configuration file.
        explicit SimulationServer(const std::string &configPath);
        /// Stops the server thread and releases owned physics resources.
        ~SimulationServer();

        /// Starts the simulation loop thread.
        void start();
        /// Stops the simulation loop thread.
        void stop();

        /// Forces the paused state seen by the update loop.
        void setPaused(bool paused);
        /// Reports whether the update loop is currently paused.
        bool isPaused() const;
        /// Toggles the paused state.
        void togglePaused();
        /// Queues exactly one deterministic step while paused.
        void stepOnce();

        /// Rebuilds the runtime with a new particle count.
        void setParticleCount(std::uint32_t particleCount);
        /// Sets the simulation timestep in seconds.
        void setDt(float dt);
        /// Multiplies the current timestep by `factor`.
        void scaleDt(float factor);
        /// Returns the currently configured timestep in seconds.
        float getDt() const;

        /// Reinitializes the active scenario using the current configuration.
        void requestReset();
        /// Attempts recovery from a latched runtime fault.
        void requestRecover();
        /// Selects the gravity solver implementation.
        void setSolverMode(const std::string &mode);
        /// Selects the numerical integrator implementation.
        void setIntegratorMode(const std::string &mode);
        /// Applies a named performance profile.
        void setPerformanceProfile(const std::string &profile);
        /// Updates octree approximation parameters.
        void setOctreeParameters(float theta, float softening);
        /// Enables or disables SPH processing.
        void setSphEnabled(bool enabled);
        /// Updates the SPH model parameters in SI units.
        void setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity);
        /// Configures deterministic substep sizing.
        void setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps);
        /// Sets how often snapshots are published to remote clients.
        void setSnapshotPublishPeriodMs(std::uint32_t periodMs);
        /// Replaces the procedural initial-state configuration.
        void setInitialStateConfig(const InitialStateConfig &config);
        /// Configures energy measurement cadence and retention.
        void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit);
        /// Enables or disables bounded GPU telemetry sampling.
        void setGpuTelemetryEnabled(bool enabled);
        /// Sets default output parameters for snapshot export.
        void setExportDefaults(const std::string &directory, const std::string &format);
        /// Updates the frontend-advertised snapshot draw budget used for transfer trimming.
        void setSnapshotTransferCap(std::uint32_t maxPoints);
        /// Selects a file-backed initial state and its parsing format.
        void setInitialStateFile(const std::string &path, const std::string &format);
        /// Queues an export of the current runtime state.
        void requestExportSnapshot(const std::string &outputPath, const std::string &format);

        /// Moves the newest snapshot into `outSnapshot` when one is available.
        bool tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot);
        /// Copies the newest published snapshot into `outSnapshot`.
        bool copyLatestSnapshot(
            std::vector<RenderParticle> &outSnapshot,
            std::size_t maxPoints = 0,
            std::size_t *outSourceSize = nullptr) const;
        /// Returns the latest authoritative telemetry sample.
        SimulationStats getStats() const;
        /// Returns the current configuration mirror used by the runtime.
        SimulationConfig getRuntimeConfig() const;

    private:
        struct EnergyValues {
            float kinetic;
            float potential;
            float thermal;
            float radiated;
            float total;
            bool estimated;
        };

        void loop();
        void publishSnapshot();
        void rebuildSystem();
        EnergyValues computeEnergyValues();
        void maybeUpdateEnergy(std::uint64_t currentStep);
        void clearGpuTelemetry();
        void maybeSampleGpuTelemetry(std::string_view solverMode, std::uint64_t currentStep);
        void processPendingExport();
        bool exportCurrentState(const std::string &outputPath, const std::string &format);
        bool loadInitialState(std::vector<Particle> &outParticles, const std::string &inputPath, const std::string &format) const;
        void clearPublishedSnapshotCache();

        std::atomic<bool> _running;
        std::atomic<bool> _paused;
        std::atomic<bool> _resetRequested;
        std::atomic<bool> _exportRequested;
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

        std::string _pendingExportPath;
        std::string _pendingExportFormat;
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
        std::vector<RenderParticle> _publishedSnapshot;
        std::vector<RenderParticle> _scratchSnapshot;
        std::string _faultReason;
        std::unique_ptr<ParticleSystem> _system;

        static constexpr std::uint64_t kGpuTelemetrySampleStride = 8u;
};





#endif // GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONSERVER_HPP_
