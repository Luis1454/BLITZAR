#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIG_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIG_HPP_

/*
 * Module: config
 * Responsibility: Define the persistent simulation and client configuration surface.
 */

#include <cstdint>
#include <string>

/// Stores the persisted runtime, rendering, export, and scenario initialization parameters.
struct SimulationConfig {
    // Physical quantities use SI units unless a field name explicitly carries another unit such as ms or fps.
    std::uint32_t particleCount = 10000u;
    float dt = 0.01f;
    std::string solver = "pairwise_cuda";
    std::string integrator = "euler";
    std::string performanceProfile = "interactive";
    std::string simulationProfile = "disk_orbit";
    float substepTargetDt = 0.01f;
    std::uint32_t maxSubsteps = 4u;
    std::uint32_t snapshotPublishPeriodMs = 50u;
    float octreeTheta = 1.2f;
    float octreeSoftening = 2.5f;
    std::string octreeOpeningCriterion = "com";
    bool octreeThetaAutoTune = false;
    float octreeThetaAutoMin = 0.4f;
    float octreeThetaAutoMax = 1.2f;
    std::uint32_t clientParticleCap = 4096u;
    float defaultZoom = 8.0f;
    int defaultLuminosity = 100;
    std::string uiTheme = "light";
    std::uint32_t uiFpsLimit = 60u;
    std::uint32_t clientRemoteCommandTimeoutMs = 80u;
    std::uint32_t clientRemoteStatusTimeoutMs = 40u;
    std::uint32_t clientRemoteSnapshotTimeoutMs = 140u;
    std::uint32_t clientSnapshotQueueCapacity = 4u;
    std::string clientSnapshotDropPolicy = "latest-only";
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
    bool deterministicMode = false;
    float physicsMaxAcceleration = 64.0f;
    float physicsMinSoftening = 1e-4f;
    float physicsMinDistance2 = 1e-12f;
    float physicsMinTheta = 0.05f;
    float sphMaxAcceleration = 40.0f;
    float sphMaxSpeed = 120.0f;
    bool renderCullingEnabled = true;
    bool renderLODEnabled = true;
    float renderLODNearDistance = 10.0f;
    float renderLODFarDistance = 60.0f;

    /// Returns the repository default configuration used for new projects and tests.
    static SimulationConfig defaults();
    /// Loads the configuration at `path` or writes defaults there when the file does not exist.
    static SimulationConfig loadOrCreate(const std::string &path);
    /// Persists the current configuration to `path`.
    bool save(const std::string &path) const;
};


#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIG_HPP_
