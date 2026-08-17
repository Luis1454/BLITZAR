/*
 * @file engine/server/simulation/state/SrvStateInternal.hpp
 * @brief Internal simulation-state contracts.
 */

#ifndef BLITZAR_ENGINE_SERVER_SIMULATION_STATE_SRV_STATE_INTERNAL_HPP_
#define BLITZAR_ENGINE_SERVER_SIMULATION_STATE_SRV_STATE_INTERNAL_HPP_

#include "server/simulation/runtime/SrvRuntimeBase.hpp"

void atomicAddFloat(std::atomic<float>& atom, float val);

#endif
