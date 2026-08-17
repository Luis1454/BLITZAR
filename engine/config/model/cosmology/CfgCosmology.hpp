/*
 * @file engine/config/CfgCosmology.hpp
 * @brief Reproducible cosmology configuration contract.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_COSMOLOGY_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_COSMOLOGY_HPP_

#include <algorithm>
#include <cstdint>
#include <string>

struct CosmologyConfig final {
    bool enabled = false;
    // "expanding_preview" preserves the historical visualisation path. "comoving" is the
    // periodic PM-only scientific path with canonical momenta.
    std::string mode = "expanding_preview";
    std::string geometry = "sphere";
    float boxHalfExtent = 48.0f;
    float sphereRadius = 48.0f;
    float hubbleH0 = 0.07f;
    float omegaMatter = 0.315f;
    float omegaLambda = 0.68491f;
    float omegaRadiation = 0.00009f;
    float initialScaleFactor = 0.01f;
    float perturbationAmplitude = 0.01f;
    float peculiarVelocityScale = 1.0f;
    std::string massModel = "critical_density";
    float totalMass = 0.0f;
};

inline bool isComovingCosmology(const CosmologyConfig& config)
{
    return config.enabled && config.mode == "comoving";
}

inline float cosmologyDomainVolume(const CosmologyConfig& config)
{
    constexpr float pi = 3.14159265358979323846f;
    if (config.geometry == "cube") {
        const float extent = std::max(0.0f, config.boxHalfExtent);
        return 8.0f * extent * extent * extent;
    }
    const float radius = std::max(0.0f, config.sphereRadius);
    return (4.0f / 3.0f) * pi * radius * radius * radius;
}

inline float cosmologyReferenceTotalMass(const CosmologyConfig& config, float particleMass,
                                         std::uint32_t particleCount)
{
    if (config.massModel == "particle_mass") {
        return std::max(0.0f, particleMass) * static_cast<float>(particleCount);
    }
    if (config.massModel == "total_mass" && config.totalMass > 0.0f) {
        return config.totalMass;
    }
    constexpr float pi = 3.14159265358979323846f;
    const float criticalDensity =
        3.0f * config.hubbleH0 * config.hubbleH0 / (8.0f * pi);
    return std::max(0.0f, config.omegaMatter) * std::max(0.0f, criticalDensity) *
           cosmologyDomainVolume(config);
}

inline float resolveCosmologyParticleMass(const CosmologyConfig& config, float particleMass,
                                          std::uint32_t particleCount)
{
    const std::uint32_t count = std::max<std::uint32_t>(1u, particleCount);
    const float totalMass = cosmologyReferenceTotalMass(config, particleMass, count);
    if (totalMass <= 0.0f) {
        return std::max(1.0e-12f, particleMass);
    }
    return std::max(1.0e-12f, totalMass / static_cast<float>(count));
}

#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_COSMOLOGY_HPP_
