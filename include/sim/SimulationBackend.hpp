#ifndef SIMULATIONBACKEND_HPP_
#define SIMULATIONBACKEND_HPP_

#include "core/ParticleSystem.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct RenderParticle {
    float x;
    float y;
    float z;
    float mass;
    float pressureNorm;
    float temperature;
};

struct InitialStateConfig {
    std::string mode = "disk_orbit";
    std::uint32_t seed = 42u;
    float velocityTemperature = 0.0f;
    float particleTemperature = 0.0f;
    float thermalAmbientTemperature = 0.0f;
    float thermalSpecificHeat = 1.0f;
    float thermalHeatingCoeff = 0.0f;
    float thermalRadiationCoeff = 0.0f;
    bool includeCentralBody = true;
    float centralMass = 1.0f;
    float centralX = 0.0f;
    float centralY = 0.0f;
    float centralZ = 0.0f;
    float centralVx = 0.0f;
    float centralVy = 0.0f;
    float centralVz = 0.0f;
    float diskMass = 0.75f;
    float diskRadiusMin = 1.5f;
    float diskRadiusMax = 11.5f;
    float diskThickness = 0.0f;
    float velocityScale = 1.0f;
    float cloudHalfExtent = 12.0f;
    float cloudSpeed = 0.0f;
    float particleMass = 0.01f;
};

struct SimulationStats {
    std::uint64_t steps;
    float dt;
    bool paused;
    bool sphEnabled;
    float backendFps;
    std::uint32_t particleCount;
    float kineticEnergy;
    float potentialEnergy;
    float thermalEnergy;
    float radiatedEnergy;
    float totalEnergy;
    float energyDriftPct;
    bool energyEstimated;
    const char *solverName;
};

class SimulationBackend {
    public:
        explicit SimulationBackend(std::uint32_t particleCount, float initialDt);
        ~SimulationBackend();

        void start();
        void stop();

        void setPaused(bool paused);
        bool isPaused() const;
        void togglePaused();
        void stepOnce();

        void setParticleCount(std::uint32_t particleCount);
        void setDt(float dt);
        void scaleDt(float factor);
        float getDt() const;

        void requestReset();
        void setSolverMode(const std::string &mode);
        void setIntegratorMode(const std::string &mode);
        void setOctreeParameters(float theta, float softening);
        void setSphEnabled(bool enabled);
        void setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity);
        void setInitialStateConfig(const InitialStateConfig &config);
        void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit);
        void setExportDefaults(const std::string &directory, const std::string &format);
        void setInitialStateFile(const std::string &path, const std::string &format);
        void requestExportSnapshot(const std::string &outputPath, const std::string &format);

        bool tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot);
        SimulationStats getStats() const;

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
        EnergyValues computeEnergyValues() const;
        void maybeUpdateEnergy(std::uint64_t currentStep);
        void processPendingExport();
        bool exportCurrentState(const std::string &outputPath, const std::string &format) const;
        bool loadInitialState(std::vector<Particle> &outParticles, const std::string &inputPath, const std::string &format) const;

        std::atomic<bool> _running;
        std::atomic<bool> _paused;
        std::atomic<bool> _resetRequested;
        std::atomic<bool> _exportRequested;
        std::atomic<std::uint32_t> _stepRequests;

        std::atomic<float> _dt;
        std::atomic<std::uint64_t> _steps;
        std::atomic<float> _backendFps;

        std::atomic<float> _kineticEnergy;
        std::atomic<float> _potentialEnergy;
        std::atomic<float> _thermalEnergy;
        std::atomic<float> _radiatedEnergy;
        std::atomic<float> _totalEnergy;
        std::atomic<float> _energyDriftPct;
        std::atomic<bool> _energyEstimated;
        std::atomic<std::uint32_t> _energyMeasureEverySteps;
        std::atomic<std::uint32_t> _energySampleLimit;

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
        InitialStateConfig _initialStateConfig;

        std::thread _thread;
        std::mutex _snapshotMutex;
        mutable std::mutex _commandMutex;
        std::vector<RenderParticle> _publishedSnapshot;
        std::vector<RenderParticle> _scratchSnapshot;
        ParticleSystem *_system;
};

#endif /* !SIMULATIONBACKEND_HPP_ */

