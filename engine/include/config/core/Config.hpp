/*
 * @file engine/include/config/core/Config.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIG_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIG_HPP_
/*
 * Module: config
 * Responsibility: Define the persistent simulation and client configuration
 * surface.
 */
#include "Constants.hpp"
#include "config/Cosmology.hpp"
#include "config/Scene.hpp"
#include <cstdint>
#include <string>

/*
 * @brief Defines the simulation config type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationConfig {
    // Physical quantities use SI units unless a field name explicitly carries another unit such as
    // ms or fps.
    std::uint32_t particleCount = 10000u;
    float dt = kDefaultSimulationDt;
    std::string solver = "pairwise_cuda";
    std::string integrator = "euler";
    std::string performanceProfile = "interactive";
    std::string simulationProfile;
    float substepTargetDt = 0.0f;
    std::uint32_t maxSubsteps = 4u;
    bool adaptiveTimeStepsEnabled = false;
    std::uint32_t adaptiveTimeStepMaxLevel = 4u;
    float adaptiveTimeStepEta = 0.25f;
    bool adaptiveTimeStepCostGuard = true;
    std::uint32_t snapshotPublishPeriodMs = 50u;
    float octreeTheta = 1.2f;
    float octreeSoftening = 2.5f;
    std::string octreeOpeningCriterion = "com";
    bool octreeThetaAutoTune = false;
    float octreeThetaAutoMin = 0.4f;
    float octreeThetaAutoMax = 1.2f;
    bool treePmEnabled = false;
      std::string treePmPreset = "custom";
      std::string treePmModel = "auto";
      std::string treePmLayout = "auto";
      std::string treePmPrecision = "fp32";
      std::string treePmAssignment = "cic";
      bool treePmLocalGrid = true;
    std::uint32_t treePmGridSize = 64u;
    std::uint32_t treePmJacobiIterations = 12u;
    float treePmCutoffFactor = 1.0f;
    std::uint32_t treePmMaxLocalNeighbors = 64u;
    std::uint32_t treePmParticleLimit = 0u;
    std::uint32_t treePmDenseCellThreshold = 64u;
    bool treePmGravityOnlyBuffers = true;
    std::uint32_t linearOctreeLeafCapacity = 256u;
    std::string cudaCachePreference = "l1";
    std::uint32_t clientParticleCap = 4096u;
    float defaultZoom = kDefaultZoom;
    int defaultLuminosity = kDefaultLuminosity;
    std::string uiTheme = "light";
    std::uint32_t uiFpsLimit = 60u;
    std::uint32_t clientRemoteCommandTimeoutMs = kRuntimeRemoteCommandTimeoutDefaultMs;
    std::uint32_t clientRemoteStatusTimeoutMs = kRuntimeRemoteStatusTimeoutDefaultMs;
    std::uint32_t clientRemoteSnapshotTimeoutMs = kRuntimeRemoteSnapshotTimeoutDefaultMs;
    std::uint32_t clientSnapshotQueueCapacity = 4u;
    std::string clientSnapshotDropPolicy = "latest-only";
    std::string exportDirectory = "exports";
    std::string exportFormat = "vtk";
    std::string inputFile;
    std::string inputFormat = "auto";
    bool cosmologyEnabled = false;
    std::string cosmologyMode = "expanding_preview";
    std::string cosmologyGeometry = "sphere";
    float cosmologyBoxHalfExtent = 48.0f;
    float cosmologySphereRadius = 48.0f;
    float cosmologyHubbleH0 = 0.07f;
    float cosmologyOmegaMatter = 0.315f;
    float cosmologyOmegaLambda = 0.68491f;
    float cosmologyOmegaRadiation = 0.00009f;
    float cosmologyInitialScaleFactor = 0.01f;
    float cosmologyPerturbationAmplitude = 0.01f;
    float cosmologyPeculiarVelocityScale = 1.0f;
    std::string cosmologyMassModel = "critical_density";
    float cosmologyTotalMass = 0.0f;
    SceneConfig scene;
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
    float initCubeHalfExtent = 12.0f;
    float initSphereRadius = 12.0f;
    float initCloudSpeed = 0.0f;
    float initParticleMass = 0.01f;
    float sceneOffsetX = 0.0f;
    float sceneOffsetY = 0.0f;
    float sceneOffsetZ = 0.0f;
    float sceneRotationX = 0.0f;
    float sceneRotationY = 0.0f;
    float sceneRotationZ = 0.0f;
    std::string sceneCopyAxis = "z";
    std::uint32_t sceneRotationCopies = 1u;
    bool sceneMirrorX = false;
    bool sceneMirrorY = false;
    bool sceneMirrorZ = false;
    bool sphEnabled = false;
    float sphSmoothingLength = 1.25f;
    float sphRestDensity = 1.0f;
    float sphGasConstant = 4.0f;
    float sphViscosity = 0.08f;
    std::uint32_t energyMeasureEverySteps = 120u;
    std::uint32_t energySampleLimit = 256u;
    bool deterministicMode = false;
    float physicsMaxAcceleration = kPhysicsMaxAccelerationDefault;
    float physicsMinSoftening = kPhysicsMinSofteningDefault;
    float physicsMinDistance2 = kPhysicsMinDistance2Default;
    float physicsMinTheta = kPhysicsMinTheta;
    float sphMaxAcceleration = kSphMaxAccelerationDefault;
    float sphMaxSpeed = kSphMaxSpeedDefault;
    bool renderCullingEnabled = true;
    bool renderLODEnabled = true;
    float renderLODNearDistance = kRenderLODNearDistance;
    float renderLODFarDistance = kRenderLODFarDistance;
    /*
     * @brief Documents the defaults operation contract.
     * @param None This contract does not take explicit parameters.
     * @return SimulationConfig value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static SimulationConfig defaults();
    /*
     * @brief Documents the load or create operation contract.
     * @param path Input value used by this contract.
     * @return SimulationConfig value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static SimulationConfig loadOrCreate(const std::string& path);
    /*
     * @brief Load an existing configuration without creating or repairing it.
     * @param path Existing INI path.
     * @param outConfig Destination for the parsed configuration.
     * @param outError Human-readable parse or validation diagnostics.
     * @return true only when the file is readable and valid for a run.
     */
    static bool loadStrict(const std::string& path, SimulationConfig& outConfig,
                           std::string& outError);
    /*
     * @brief Documents the save operation contract.
     * @param path Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool save(const std::string& path) const;
};
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONCONFIG_HPP_
