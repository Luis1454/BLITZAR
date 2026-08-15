/*
 * @file engine/include/types/SimulationTypes.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Source artifact for the BLITZAR simulation project.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_TYPES_SIMULATIONTYPES_HPP_
#define BLITZAR_ENGINE_INCLUDE_TYPES_SIMULATIONTYPES_HPP_
#include "config/Scene.hpp"
#include "config/Cosmology.hpp"
#include <cstdint>
#include <string>

/*
 * @brief Defines the render particle type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct RenderParticle {
    float x;
    float y;
    float z;
    float mass;
    float pressureNorm;
    float temperature;
    // Normalized coarse-grained mass density for visualization. Particle mass remains conserved.
    float densityNorm = 0.0f;
};

/*
 * @brief Defines the initial state config type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct InitialStateConfig {
    SceneConfig scene;
    CosmologyConfig cosmology;
    std::string mode = "disk_orbit";
    std::uint32_t seed = 42u;
    bool deterministicMode = false;
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
    float cubeHalfExtent = 12.0f;
    float sphereRadius = 12.0f;
    float cloudSpeed = 0.0f;
    float particleMass = 0.01f;
    float sceneOffsetX = 0.0f;
    float sceneOffsetY = 0.0f;
    float sceneOffsetZ = 0.0f;
    float scenePivotX = 0.0f;
    float scenePivotY = 0.0f;
    float scenePivotZ = 0.0f;
    float sceneRotationX = 0.0f;
    float sceneRotationY = 0.0f;
    float sceneRotationZ = 0.0f;
    std::string sceneCopyAxis = "z";
    std::uint32_t sceneRotationCopies = 1u;
    bool sceneMirrorX = false;
    bool sceneMirrorY = false;
    bool sceneMirrorZ = false;
};

/*
 * @brief Defines the simulation stats type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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
    float totalMass;
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
#endif // BLITZAR_ENGINE_INCLUDE_TYPES_SIMULATIONTYPES_HPP_
