/*
 * @file engine/server/src/simulation/state/InitializationHelper.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Helper utilities for deterministic parallel particle generation.
 */

#ifndef BLITZAR_ENGINE_SRC_SERVER_SIMULATION_STATE_INITIALIZATION_HELPER_HPP_
#define BLITZAR_ENGINE_SRC_SERVER_SIMULATION_STATE_INITIALIZATION_HELPER_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>

/*
 * @brief Pre-generated random data for parallel consumption.
 * Ensures deterministic output while allowing parallel particle fabrication.
 */
struct RandomData {
    std::vector<float> valuesF;
    std::size_t elementSize;

    inline RandomData() : elementSize(0) {}

    inline void reserve(std::size_t numParticles, std::size_t valuesPerParticle) {
        elementSize = valuesPerParticle;
        valuesF.reserve(numParticles * valuesPerParticle);
    }

    inline void push(float v) { valuesF.push_back(v); }

    inline float get(std::size_t particleIndex, std::size_t valueIndex) const {
        return valuesF[particleIndex * elementSize + valueIndex];
    }

    inline std::size_t particleCount() const {
        return elementSize > 0 ? valuesF.size() / elementSize : 0;
    }
};

#endif // BLITZAR_ENGINE_SRC_SERVER_SIMULATION_STATE_INITIALIZATION_HELPER_HPP_
