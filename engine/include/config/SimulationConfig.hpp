#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIG_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIG_HPP_

#include "protocol/ServerProtocol.hpp"

#include <cstdint>
#include <string>

struct SimulationConfig {
    std::uint32_t particleCount = 10000u;
    float dt = 0.01f;
    std::string solver = "pairwise_cuda";
    std::string integrator = "euler";
    std::string performanceProfile = "interactive";
    float substepTargetDt = 0.01f;
    std::uint32_t maxSubsteps = 4u;
    std::uint32_t snapshotPublishPeriodMs = 50u;
    float octreeTheta = 1.2f;
    float octreeSoftening = 2.5f;
    std::uint32_t clientParticleCap = grav_protocol::kSnapshotDefaultPoints;
    float defaultZoom = 8.0f;
    int defaultLuminosity = 100;
    std::uint32_t uiFpsLimit = 60u;
    std::uint32_t clientRemoteCommandTimeoutMs = 80u;
    std::uint32_t clientRemoteStatusTimeoutMs = 40u;
    std::uint32_t clientRemoteSnapshotTimeoutMs = 140u;
    std::string exportDirectory = "exports";
    std::string exportFormat = "vtk";
    std::string inputFile;
    std::string inputFormat = "auto";
    std::string initConfigStyle = "preset";
    std::string presetStructure = "disk_orbit";
    float presetSize = 12.0f;
    float velocityTemperature = 0.0f;
    float particleTemperature = 0.0f;
    float thermalAmbientTemperature = 0.0f;
    float thermalSpecificHeat = 1.0f;
    float thermalHeatingCoeff = 0.0f;
    float thermalRadiationCoeff = 0.0f;
    std::string initMode = "disk_orbit";
    std::uint32_t initSeed = 42u;
    bool initIncludeCentralBody = true;
    float initCentralMass = 1.0f;
    float initCentralX = 0.0f;
    float initCentralY = 0.0f;
    float initCentralZ = 0.0f;
    float initCentralVx = 0.0f;
    float initCentralVy = 0.0f;
    float initCentralVz = 0.0f;
    float initDiskMass = 0.75f;
    float initDiskRadiusMin = 1.5f;
    float initDiskRadiusMax = 11.5f;
    float initDiskThickness = 0.0f;
    float initVelocityScale = 1.0f;
    float initCloudHalfExtent = 12.0f;
    float initCloudSpeed = 0.0f;
    float initParticleMass = 0.01f;
    bool sphEnabled = false;
    float sphSmoothingLength = 1.25f;
    float sphRestDensity = 1.0f;
    float sphGasConstant = 4.0f;
    float sphViscosity = 0.08f;
    std::uint32_t energyMeasureEverySteps = 120u;
    std::uint32_t energySampleLimit = 256u;

    static SimulationConfig defaults();
    static SimulationConfig loadOrCreate(const std::string &path);
    bool save(const std::string &path) const;
};


#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIG_HPP_
