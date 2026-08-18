/*
 * @file engine/server/simulation/configuration/SrvSimulationInitConfig.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Source artifact for the BLITZAR simulation project.
 */

#include "server/simulation/configuration/SrvSimulationInitConfig.hpp"
#include "config/core/configuration/CfgConfig.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <ostream>
#include <sstream>

/*
 * @brief Documents the to lower init config operation contract.
 * @param value Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string toLowerInitConfig(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

/*
 * @brief Documents the is supported init mode operation contract.
 * @param value Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static bool isSupportedInitMode(const std::string& value)
{
    return value == "disk_orbit" || value == "cosmology" || value == "random_cloud" || value == "cube_random" ||
           value == "sphere_random" || value == "two_body" || value == "three_body" ||
           value == "plummer_sphere" || value == "galaxy" || value == "galaxy_collision" ||
           value == "binary_star" ||
           value == "solar_system" || value == "sph_collapse" || value == "objects" ||
           value == "file";
}

/*
 * @brief Documents the normalize init field operation contract.
 * @param rawValue Input value used by this contract.
 * @param fieldName Input value used by this contract.
 * @param fallbackValue Input value used by this contract.
 * @param log Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::string normalizeInitField(const std::string& rawValue, const char* fieldName,
                                      const char* fallbackValue, std::ostream& log)
{
    const std::string normalized = toLowerInitConfig(rawValue);
    const bool styleField = std::string(fieldName) == "init_config_style";
    if ((styleField && (normalized == "preset" || normalized == "detailed")) ||
        (!styleField && isSupportedInitMode(normalized))) {
        return normalized;
    }
    log << "[config] invalid " << fieldName << "=" << rawValue << ", falling back to "
        << fallbackValue << "\n";
    return fallbackValue;
}

/*
 * @brief Documents the has configured input file operation contract.
 * @param value Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static bool hasConfiguredInputFile(const std::string& value)
{
    return std::any_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) == 0;
    });
}

/*
 * @brief Documents the summarize plan operation contract.
 * @param style Input value used by this contract.
 * @param selector Input value used by this contract.
 * @param mode Input value used by this contract.
 * @param inputFile Input value used by this contract.
 * @param inputFormat Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::string summarizePlan(const std::string& style, const std::string& selector,
                                 const std::string& mode, const std::string& inputFile,
                                 const std::string& inputFormat)
{
    std::ostringstream out;
    out << "init plan style=" << style << " selector=" << selector << " mode=" << mode;
    if (mode == "file") {
        out << " source=file"
            << " input_file=" << inputFile
            << " input_format=" << (inputFormat.empty() ? "auto" : inputFormat);
    }
    else {
        out << " source=generated";
    }
    return out.str();
}

static void configureCosmology(InitialStateConfig& init, const SimulationConfig& config,
                               std::ostream& log)
{
    init.cosmology.enabled = config.cosmologyEnabled;
    init.cosmology.mode = toLowerInitConfig(config.cosmologyMode);
    if (init.cosmology.mode != "expanding_preview" && init.cosmology.mode != "comoving") {
        init.cosmology.mode = "expanding_preview";
        log << "[config] invalid cosmology mode; using expanding_preview\n";
    }
    init.cosmology.geometry = toLowerInitConfig(config.cosmologyGeometry);
    init.cosmology.boxHalfExtent = std::max(0.000001f, config.cosmologyBoxHalfExtent);
    init.cosmology.sphereRadius = std::max(0.000001f, config.cosmologySphereRadius);
    init.cosmology.hubbleH0 = std::max(0.0f, config.cosmologyHubbleH0);
    init.cosmology.omegaMatter = std::max(0.0f, config.cosmologyOmegaMatter);
    init.cosmology.omegaLambda = std::max(0.0f, config.cosmologyOmegaLambda);
    init.cosmology.omegaRadiation = std::max(0.0f, config.cosmologyOmegaRadiation);
    init.cosmology.initialScaleFactor =
        std::max(0.000001f, config.cosmologyInitialScaleFactor);
    init.cosmology.perturbationAmplitude =
        std::clamp(config.cosmologyPerturbationAmplitude, 0.0f, 1.0f);
    init.cosmology.peculiarVelocityScale = std::max(0.0f, config.cosmologyPeculiarVelocityScale);
    init.cosmology.massModel = toLowerInitConfig(config.cosmologyMassModel);
    if (init.cosmology.massModel != "critical_density" &&
        init.cosmology.massModel != "total_mass" && init.cosmology.massModel != "particle_mass") {
        init.cosmology.massModel = "critical_density";
        log << "[config] invalid cosmology mass_model; using critical_density\n";
    }
    init.cosmology.totalMass = std::max(0.0f, config.cosmologyTotalMass);
}

static void configureSharedInit(InitialStateConfig& init, const SimulationConfig& config)
{
    init.scene = config.scene;
    init.seed = config.initSeed;
    init.deterministicMode = config.deterministicMode;
    init.velocityTemperature = std::max(0.0f, config.velocityTemperature);
    init.particleTemperature = std::max(0.0f, config.particleTemperature);
    init.sceneOffsetX = config.sceneOffsetX;
    init.sceneOffsetY = config.sceneOffsetY;
    init.sceneOffsetZ = config.sceneOffsetZ;
    init.sceneRotationX = config.sceneRotationX;
    init.sceneRotationY = config.sceneRotationY;
    init.sceneRotationZ = config.sceneRotationZ;
    init.sceneCopyAxis = toLowerInitConfig(config.sceneCopyAxis);
    if (init.sceneCopyAxis != "x" && init.sceneCopyAxis != "y" && init.sceneCopyAxis != "z") {
        init.sceneCopyAxis = "z";
    }
    init.sceneRotationCopies = std::clamp(config.sceneRotationCopies, 1u, 256u);
    init.sceneMirrorX = config.sceneMirrorX;
    init.sceneMirrorY = config.sceneMirrorY;
    init.sceneMirrorZ = config.sceneMirrorZ;
    init.thermalAmbientTemperature = std::max(0.0f, config.thermalAmbientTemperature);
    init.thermalSpecificHeat = std::max(1e-6f, config.thermalSpecificHeat);
    init.thermalHeatingCoeff = std::max(0.0f, config.thermalHeatingCoeff);
    init.thermalRadiationCoeff = std::max(0.0f, config.thermalRadiationCoeff);
}

static float defaultCloudParticleMass(const SimulationConfig& config)
{
    const std::uint32_t count = std::max<std::uint32_t>(2u, config.particleCount);
    return std::max(1e-6f, 1.0f / static_cast<float>(count));
}

static void clearGeneratedCentralBody(InitialStateConfig& init)
{
    init.includeCentralBody = false;
    init.centralMass = 1.0f;
    init.centralX = 0.0f;
    init.centralY = 0.0f;
    init.centralZ = 0.0f;
    init.centralVx = 0.0f;
    init.centralVy = 0.0f;
    init.centralVz = 0.0f;
}

static void configurePresetMode(InitialStateConfig& init, const SimulationConfig& config,
                                const std::string& mode, float size)
{
    init.mode = mode;
    if (mode == "cosmology") {
        init.cosmology.enabled = true;
        init.includeCentralBody = false;
        init.centralMass = 0.0f;
        init.cloudHalfExtent = size;
        init.cubeHalfExtent = size;
        init.sphereRadius = size;
        init.particleMass = std::max(1e-12f, config.initParticleMass);
        return;
    }
    if (mode == "random_cloud" || mode == "cube_random" || mode == "sphere_random") {
        clearGeneratedCentralBody(init);
        init.cloudHalfExtent = size;
        init.cloudSpeed = 0.0f;
        init.particleMass = defaultCloudParticleMass(config);
        if (mode == "random_cloud") {
            init.cubeHalfExtent = size;
            init.sphereRadius = size;
        }
        if (mode == "cube_random") {
            init.cubeHalfExtent = size;
        }
        if (mode == "sphere_random") {
            init.sphereRadius = size;
        }
        return;
    }
    if (mode == "two_body" || mode == "three_body" || mode == "binary_star") {
        init.includeCentralBody = false;
        init.centralMass = 0.0f;
        init.cloudHalfExtent = size;
        init.velocityScale = 1.0f;
        init.particleMass = 1.0f;
        return;
    }
    if (mode == "plummer_sphere") {
        init.includeCentralBody = false;
        init.centralMass = 0.0f;
        init.cloudHalfExtent = size;
        init.velocityScale = 1.0f;
        init.particleMass = defaultCloudParticleMass(config);
        return;
    }
    if (mode == "galaxy" || mode == "galaxy_collision") {
        init.includeCentralBody = false;
        init.centralMass = 0.0f;
        init.cloudHalfExtent = size;
        init.velocityScale = 1.0f;
        init.diskRadiusMin = std::max(0.05f, size * 0.1f);
        init.diskRadiusMax = size;
        init.diskMass = std::max(1e-6f, config.initDiskMass);
        init.particleMass = std::max(1e-6f, config.initParticleMass);
        return;
    }
    if (mode == "solar_system") {
        init.includeCentralBody = true;
        init.centralMass = std::max(1e-6f, config.initCentralMass);
        init.velocityScale = std::max(0.0f, config.initVelocityScale);
        return;
    }
    if (mode == "sph_collapse") {
        init.includeCentralBody = false;
        init.centralMass = 0.0f;
        init.cloudHalfExtent = size;
        init.particleMass = defaultCloudParticleMass(config);
        return;
    }
    if (mode == "file") {
        return;
    }
    init.mode = "disk_orbit";
    init.includeCentralBody = true;
    init.centralMass = 1.0f;
    init.diskMass = 0.75f;
    init.diskRadiusMin = std::max(0.05f, size * 0.15f);
    init.diskRadiusMax = std::max(init.diskRadiusMin + 0.01f, size);
    init.diskThickness = size * 0.01f;
    init.velocityScale = 1.0f;
    const std::uint32_t diskCount =
        std::max<std::uint32_t>(1u, std::max<std::uint32_t>(2u, config.particleCount) - 1u);
    init.particleMass = std::max(1e-6f, init.diskMass / static_cast<float>(diskCount));
}

static void configureDetailedMode(InitialStateConfig& init, const SimulationConfig& config,
                                  const std::string& mode)
{
    init.mode = mode;
    if (mode == "cosmology") {
        init.cosmology.enabled = true;
    }
    init.includeCentralBody = config.initIncludeCentralBody;
    init.centralMass = config.initCentralMass;
    init.centralX = config.initCentralX;
    init.centralY = config.initCentralY;
    init.centralZ = config.initCentralZ;
    init.centralVx = config.initCentralVx;
    init.centralVy = config.initCentralVy;
    init.centralVz = config.initCentralVz;
    init.diskMass = config.initDiskMass;
    init.diskRadiusMin = config.initDiskRadiusMin;
    init.diskRadiusMax = config.initDiskRadiusMax;
    init.diskThickness = config.initDiskThickness;
    init.velocityScale = config.initVelocityScale;
    init.cloudHalfExtent = config.initCloudHalfExtent;
    init.cubeHalfExtent = config.initCubeHalfExtent;
    init.sphereRadius = config.initSphereRadius;
    init.cloudSpeed = config.initCloudSpeed;
    init.particleMass = config.initParticleMass;
}

static std::string resolveInputPlan(ResolvedInitialStatePlan& plan, const SimulationConfig& config,
                                    const std::string& selector, std::string resolvedMode,
                                    std::ostream& log)
{
    if (resolvedMode == "file") {
        if (!hasConfiguredInputFile(config.inputFile)) {
            log << "[config] " << selector
                << "=file requires non-empty input_file, falling back to disk_orbit\n";
            return "disk_orbit";
        }
        plan.inputFile = config.inputFile;
        plan.inputFormat = config.inputFormat.empty() ? "auto" : config.inputFormat;
    }
    else if (hasConfiguredInputFile(config.inputFile)) {
        log << "[config] input_file ignored because resolved init mode is " << resolvedMode << "\n";
    }
    return resolvedMode;
}

/*
 * @brief Documents the resolve initial state plan operation contract.
 * @param config Input value used by this contract.
 * @param log Input value used by this contract.
 * @return ResolvedInitialStatePlan value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ResolvedInitialStatePlan resolveInitialStatePlan(const SimulationConfig& config, std::ostream& log)
{
    ResolvedInitialStatePlan plan;
    InitialStateConfig& init = plan.config;
    configureCosmology(init, config, log);
    configureSharedInit(init, config);
    const SimulationConfig defaults = SimulationConfig::defaults();
    const std::string style =
        normalizeInitField(config.initConfigStyle, "init_config_style", "preset", log);
    const std::string preset =
        normalizeInitField(config.presetStructure, "preset_structure", "disk_orbit", log);
    const std::string detailed =
        normalizeInitField(config.initMode, "init_mode", "disk_orbit", log);
    const bool presetSelected = style == "preset";
    const std::string selector = presetSelected ? "preset_structure" : "init_mode";
    if (!config.scene.objects.empty()) {
        init.mode = "objects";
        plan.summary = summarizePlan(style, "scene.objects", "objects", {}, "");
        return plan;
    }
    std::string resolvedMode = presetSelected ? preset : detailed;
    if (presetSelected && detailed != toLowerInitConfig(defaults.initMode) && detailed != preset) {
        log << "[config] init_mode=" << config.initMode
            << " ignored because init_config_style=preset selects preset_structure\n";
    }
    if (!presetSelected && preset != toLowerInitConfig(defaults.presetStructure) &&
        preset != detailed) {
        log << "[config] preset_structure=" << config.presetStructure
            << " ignored because init_config_style=detailed selects init_mode\n";
    }
    resolvedMode = resolveInputPlan(plan, config, selector, resolvedMode, log);
    if (presetSelected) {
        const float size = std::max(0.1f, config.presetSize);
        configurePresetMode(init, config, resolvedMode, size);
    }
    else {
        configureDetailedMode(init, config, resolvedMode);
    }
    if (init.mode == "cosmology") {
        init.particleMass = resolveCosmologyParticleMass(
            init.cosmology, init.particleMass, std::max<std::uint32_t>(2u, config.particleCount));
    }
    plan.summary = summarizePlan(style, selector, init.mode, plan.inputFile, plan.inputFormat);
    return plan;
}

/*
 * @brief Documents the build initial state config operation contract.
 * @param config Input value used by this contract.
 * @return InitialStateConfig value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
InitialStateConfig buildInitialStateConfig(const SimulationConfig& config)
{
    return resolveInitialStatePlan(config, std::cerr).config;
}
