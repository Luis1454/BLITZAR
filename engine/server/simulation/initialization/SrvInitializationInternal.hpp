/*
 * @file engine/server/simulation/initialization/SrvInitializationInternal.hpp
 * @brief Internal initial-state generation contracts.
 */

#ifndef BLITZAR_ENGINE_SERVER_SIMULATION_INITIALIZATION_SRV_INITIALIZATION_INTERNAL_HPP_
#define BLITZAR_ENGINE_SERVER_SIMULATION_INITIALIZATION_SRV_INITIALIZATION_INTERNAL_HPP_

#include "server/simulation/runtime/SrvRuntimeBase.hpp"

bool buildGeneratedState(std::vector<Particle>& outParticles, std::uint32_t particleCount,
                         const InitialStateConfig& config);
bool applyInitialStateTransform(std::vector<Particle>& particles,
                                const InitialStateConfig& config);

#endif
