/*
 * @file engine/src/server/simulation/state/InitializationHelper.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Helper utilities for deterministic parallel particle generation.
 */

#pragma once

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

    RandomData() : elementSize(0) {}

    void reserve(std::size_t numParticles, std::size_t valuesPerParticle) {
        elementSize = valuesPerParticle;
        valuesF.reserve(numParticles * valuesPerParticle);
    }

    void push(float v) { valuesF.push_back(v); }

    float get(std::size_t particleIndex, std::size_t valueIndex) const {
        return valuesF[particleIndex * elementSize + valueIndex];
    }

    std::size_t particleCount() const {
        return elementSize > 0 ? valuesF.size() / elementSize : 0;
    }
};
