// File: engine/include/types/SimulationTypes.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_TYPES_SIMULATIONTYPES_HPP_
#define GRAVITY_ENGINE_INCLUDE_TYPES_SIMULATIONTYPES_HPP_
#include <cstdint>
#include <string>

/// Description: Defines the RenderParticle data or behavior contract.
struct RenderParticle {
    float x;
    float y;
    float z;
    float mass;
    float pressureNorm;
    float temperature;
};

/// Description: Defines the InitialStateConfig data or behavior contract.
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

/// Description: Defines the SimulationStats data or behavior contract.
struct SimulationStats {
    std::uint64_t steps;
    float dt;
    float totalTime;
    bool paused;
    bool faulted;
    std::uint64_t faultStep;
    std::string faultReason;
    bool sphEnabled;
    float serverFps;
    std::string performanceProfile;
    float substepTargetDt;
    float substepDt;
    std::uint32_t substeps;
    std::uint32_t maxSubsteps;
    std::uint32_t snapshotPublishPeriodMs;
    std::uint32_t particleCount;
    float kineticEnergy;
    float potentialEnergy;
    float thermalEnergy;
    float radiatedEnergy;
    float totalEnergy;
    float energyDriftPct;
    bool energyEstimated;
    std::string solverName;
    std::string integratorName;
    bool gpuTelemetryEnabled;
    bool gpuTelemetryAvailable;
    float gpuKernelMs;
    float gpuCopyMs;
    std::uint64_t gpuVramUsedBytes;
    std::uint64_t gpuVramTotalBytes;
    std::uint32_t exportQueueDepth;
    bool exportActive;
    std::uint64_t exportCompletedCount;
    std::uint64_t exportFailedCount;
    std::string exportLastState;
    std::string exportLastPath;
    std::string exportLastMessage;
};
#endif // GRAVITY_ENGINE_INCLUDE_TYPES_SIMULATIONTYPES_HPP_
