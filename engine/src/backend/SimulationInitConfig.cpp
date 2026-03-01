#include "backend/SimulationInitConfig.hpp"

#include <algorithm>
#include <cctype>

std::string toLowerInitConfig(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}
InitialStateConfig buildInitialStateConfig(const SimulationConfig &config)
{
    InitialStateConfig init;
    init.seed = config.initSeed;
    init.velocityTemperature = std::max(0.0f, config.velocityTemperature);
    init.particleTemperature = std::max(0.0f, config.particleTemperature);
    init.thermalAmbientTemperature = std::max(0.0f, config.thermalAmbientTemperature);
    init.thermalSpecificHeat = std::max(1e-6f, config.thermalSpecificHeat);
    init.thermalHeatingCoeff = std::max(0.0f, config.thermalHeatingCoeff);
    init.thermalRadiationCoeff = std::max(0.0f, config.thermalRadiationCoeff);

    const std::string style = toLowerInitConfig(config.initConfigStyle);
    if (style == "preset") {
        init.mode = config.presetStructure;

        const float size = std::max(0.1f, config.presetSize);
        const std::string preset = toLowerInitConfig(config.presetStructure);
        if (preset == "random_cloud") {
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
        } else if (preset == "file") {
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
        return init;
    }

    // Detailed mode: keep explicit init_ values.
    init.mode = config.initMode;
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
    return init;
}

