/*
 * @file engine/server/simulation/state/SrvGenerationContext.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Shared state for initial particle generation strategies.
 */

#ifndef BLITZAR_ENGINE_SRC_SERVER_SIMULATION_STATE_GENERATION_CONTEXT_HPP_
#define BLITZAR_ENGINE_SRC_SERVER_SIMULATION_STATE_GENERATION_CONTEXT_HPP_

#include "simulation/SrvInternal.hpp"

#include <random>

struct GenerationContext final {
    std::vector<Particle>& particles;
    std::uint32_t count;
    const InitialStateConfig& config;
    std::mt19937 rng;
    float centralMass;
    float velocityTemperature;
    float particleTemperature;
    Vector3 centralPosition;
    Vector3 centralVelocity;

    GenerationContext(std::vector<Particle>& output, std::uint32_t particleCount,
                      const InitialStateConfig& initialConfig);

    void finalizeParticle(Particle& particle);
    void addCentralBody();
};

bool buildGeneratedStateSingle(std::vector<Particle>& particles, std::uint32_t particleCount,
                              const InitialStateConfig& config);
bool buildPrimitiveModel(GenerationContext& context, std::string_view mode);
bool buildPlummerModel(GenerationContext& context);
bool buildCosmologyModel(GenerationContext& context);
bool buildGalaxyModel(GenerationContext& context, std::string_view mode);
bool buildDiskModel(GenerationContext& context);

#endif // BLITZAR_ENGINE_SRC_SERVER_SIMULATION_STATE_GENERATION_CONTEXT_HPP_
