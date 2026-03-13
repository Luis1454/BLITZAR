#ifndef GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONSERVER_HPP_
#define GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONSERVER_HPP_

#include "physics/ParticleSystem.hpp"
#include "client/ILocalServer.hpp"
#include "config/SimulationConfig.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class SimulationServer : public grav_client::ILocalServer {
    public:
        explicit SimulationServer(std::uint32_t particleCount, float initialDt);
        explicit SimulationServer(const std::string &configPath);
        ~SimulationServer();

        void start() override;
        void stop() override;

        void setPaused(bool paused) override;
        bool isPaused() const;
        void togglePaused() override;
        void stepOnce() override;

        void setParticleCount(std::uint32_t particleCount) override;
        void setDt(float dt) override;
        void scaleDt(float factor) override;
        float getDt() const;

        void requestReset() override;
        void requestRecover();
        void setSolverMode(const std::string &mode) override;
        void setIntegratorMode(const std::string &mode) override;
        void setOctreeParameters(float theta, float softening) override;
        void setSphEnabled(bool enabled) override;
        void setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity) override;
        void setInitialStateConfig(const InitialStateConfig &config) override;
        void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit) override;
        void setExportDefaults(const std::string &directory, const std::string &format) override;
        void setInitialStateFile(const std::string &path, const std::string &format) override;
        void requestExportSnapshot(const std::string &outputPath, const std::string &format) override;

        bool tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot) override;
        bool copyLatestSnapshot(std::vector<RenderParticle> &outSnapshot, std::size_t maxPoints = 0) const;
        SimulationStats getStats() const override;
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
        void processPendingExport();
        bool exportCurrentState(const std::string &outputPath, const std::string &format);
        bool loadInitialState(std::vector<Particle> &outParticles, const std::string &inputPath, const std::string &format) const;

        std::atomic<bool> _running;
        std::atomic<bool> _paused;
        std::atomic<bool> _resetRequested;
        std::atomic<bool> _exportRequested;
        std::atomic<bool> _cudaContextDirty;
        std::atomic<std::uint32_t> _stepRequests;

        std::atomic<float> _dt;
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
        std::atomic<bool> _faulted;
        std::atomic<std::uint64_t> _faultStep;

        std::uint32_t _particleCount;
        std::string _solverMode;
        std::string _integratorMode;

        float _octreeTheta;
        float _octreeSoftening;
        bool _sphEnabled;
        float _sphSmoothingLength;
        float _sphRestDensity;
        float _sphGasConstant;
        float _sphViscosity;
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
};





#endif // GRAVITY_ENGINE_INCLUDE_SERVER_SIMULATIONSERVER_HPP_
