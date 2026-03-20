#include "server/SimulationInitConfig.hpp"

#include "config/SimulationConfig.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <ostream>
#include <sstream>

std::string toLowerInitConfig(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

static bool isSupportedInitMode(const std::string &value)
{
    return value == "disk_orbit"
        || value == "random_cloud"
        || value == "two_body"
        || value == "three_body"
        || value == "plummer_sphere"
        || value == "galaxy_collision"
        || value == "solar_system"
        || value == "sph_collapse"
        || value == "file";
}

static std::string normalizeInitField(
    const std::string &rawValue,
    const char *fieldName,
    const char *fallbackValue,
    std::ostream &log)
{
    const std::string normalized = toLowerInitConfig(rawValue);
    const bool styleField = std::string(fieldName) == "init_config_style";
    if ((styleField && (normalized == "preset" || normalized == "detailed"))
        || (!styleField && isSupportedInitMode(normalized))) {
        return normalized;
    }
    log << "[config] invalid " << fieldName << "=" << rawValue
        << ", falling back to " << fallbackValue << "\n";
    return fallbackValue;
}

static bool hasConfiguredInputFile(const std::string &value)
{
    return std::any_of(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) == 0; });
}

static std::string summarizePlan(
    const std::string &style,
    const std::string &selector,
    const std::string &mode,
    const std::string &inputFile,
    const std::string &inputFormat)
{
    std::ostringstream out;
    out << "init plan style=" << style
        << " selector=" << selector
        << " mode=" << mode;
    if (mode == "file") {
        out << " source=file"
            << " input_file=" << inputFile
            << " input_format=" << (inputFormat.empty() ? "auto" : inputFormat);
    } else {
        out << " source=generated";
    }
    return out.str();
}

ResolvedInitialStatePlan resolveInitialStatePlan(const SimulationConfig &config, std::ostream &log)
{
    ResolvedInitialStatePlan plan;
    InitialStateConfig &init = plan.config;
    init.seed = config.initSeed;
    init.velocityTemperature = std::max(0.0f, config.velocityTemperature);
    init.particleTemperature = std::max(0.0f, config.particleTemperature);
    init.thermalAmbientTemperature = std::max(0.0f, config.thermalAmbientTemperature);
    init.thermalSpecificHeat = std::max(1e-6f, config.thermalSpecificHeat);
    init.thermalHeatingCoeff = std::max(0.0f, config.thermalHeatingCoeff);
    init.thermalRadiationCoeff = std::max(0.0f, config.thermalRadiationCoeff);

    const SimulationConfig defaults = SimulationConfig::defaults();
    const std::string style = normalizeInitField(config.initConfigStyle, "init_config_style", "preset", log);
    const std::string preset = normalizeInitField(config.presetStructure, "preset_structure", "disk_orbit", log);
    const std::string detailed = normalizeInitField(config.initMode, "init_mode", "disk_orbit", log);
    const bool presetSelected = style == "preset";
    const std::string selector = presetSelected ? "preset_structure" : "init_mode";
    std::string resolvedMode = presetSelected ? preset : detailed;

    if (presetSelected && detailed != toLowerInitConfig(defaults.initMode) && detailed != preset) {
        log << "[config] init_mode=" << config.initMode
            << " ignored because init_config_style=preset selects preset_structure\n";
    }
    if (!presetSelected && preset != toLowerInitConfig(defaults.presetStructure) && preset != detailed) {
        log << "[config] preset_structure=" << config.presetStructure
            << " ignored because init_config_style=detailed selects init_mode\n";
    }

    if (resolvedMode == "file") {
        if (!hasConfiguredInputFile(config.inputFile)) {
            log << "[config] " << selector
                << "=file requires non-empty input_file, falling back to disk_orbit\n";
            resolvedMode = "disk_orbit";
        } else {
            plan.inputFile = config.inputFile;
            plan.inputFormat = config.inputFormat.empty() ? "auto" : config.inputFormat;
        }
    } else if (hasConfiguredInputFile(config.inputFile)) {
        log << "[config] input_file ignored because resolved init mode is " << resolvedMode << "\n";
    }

    if (presetSelected) {
        init.mode = resolvedMode;

        const float size = std::max(0.1f, config.presetSize);
        if (resolvedMode == "random_cloud") {
            init.includeCentralBody = false;
            init.centralMass = 1.0f;
            init.centralX = 0.0f;
            init.centralY = 0.0f;
            init.centralZ = 0.0f;
            init.centralVx = 0.0f;
            init.centralVy = 0.0f;
            init.centralVz = 0.0f;
            init.cloudHalfExtent = size;
            init.cloudSpeed = 0.0f;
            init.particleMass = std::max(1e-6f, 1.0f / static_cast<float>(std::max<std::uint32_t>(2u, config.particleCount)));
        } else if (resolvedMode == "two_body") {
            init.includeCentralBody = false;
            init.centralMass = 0.0f;
            init.cloudHalfExtent = size;
            init.velocityScale = 1.0f;
            init.particleMass = 1.0f;
        } else if (resolvedMode == "three_body") {
            init.includeCentralBody = false;
            init.centralMass = 0.0f;
            init.cloudHalfExtent = size;
            init.velocityScale = 1.0f;
            init.particleMass = 1.0f;
        } else if (resolvedMode == "plummer_sphere") {
            init.includeCentralBody = false;
            init.centralMass = 0.0f;
            init.cloudHalfExtent = size;
            init.velocityScale = 1.0f;
            init.particleMass = std::max(
                1e-6f,
                1.0f / static_cast<float>(std::max<std::uint32_t>(2u, config.particleCount))
            );
        } else if (resolvedMode == "file") {
            init.mode = "file";
        } else {
            init.mode = "disk_orbit";
            init.includeCentralBody = true;
            init.centralMass = 1.0f;
            init.centralX = 0.0f;
            init.centralY = 0.0f;
            init.centralZ = 0.0f;
            init.centralVx = 0.0f;
            init.centralVy = 0.0f;
            init.centralVz = 0.0f;
            init.diskMass = 0.75f;
            init.diskRadiusMin = std::max(0.05f, size * 0.15f);
            init.diskRadiusMax = std::max(init.diskRadiusMin + 0.01f, size);
            init.diskThickness = size * 0.01f;
            init.velocityScale = 1.0f;
            const std::uint32_t diskCount = std::max<std::uint32_t>(1u, std::max<std::uint32_t>(2u, config.particleCount) - 1u);
            init.particleMass = std::max(1e-6f, init.diskMass / static_cast<float>(diskCount));
        }
    } else {
        init.mode = resolvedMode;
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
        init.cloudSpeed = config.initCloudSpeed;
        init.particleMass = config.initParticleMass;
    }

    plan.summary = summarizePlan(style, selector, init.mode, plan.inputFile, plan.inputFormat);
    return plan;
}

InitialStateConfig buildInitialStateConfig(const SimulationConfig &config)
{
    return resolveInitialStatePlan(config, std::cerr).config;
}
