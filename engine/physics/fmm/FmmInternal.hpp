/*
 * @file engine/physics/fmm/FmmInternal.hpp
 * @brief Shared private helpers for the order-two FMM implementation.
 */

#ifndef BLITZAR_ENGINE_SRC_PHYSICS_FMMINTERNAL_HPP_
#define BLITZAR_ENGINE_SRC_PHYSICS_FMMINTERNAL_HPP_

#include "physics/fmm/FmmCpu.hpp"
#include <array>
#include <cstdint>

namespace bltzr_fmm {

struct MultiIndex final {
    int x;
    int y;
    int z;
};

inline constexpr std::array<MultiIndex, kCoefficientCount> kIndices = {
    MultiIndex{0, 0, 0}, MultiIndex{1, 0, 0}, MultiIndex{0, 1, 0}, MultiIndex{0, 0, 1},
    MultiIndex{2, 0, 0}, MultiIndex{1, 1, 0}, MultiIndex{1, 0, 1}, MultiIndex{0, 2, 0},
    MultiIndex{0, 1, 1}, MultiIndex{0, 0, 2}};

std::uint64_t cellKey(int x, int y, int z);
Vector3 cellCenter(const FmmWorkspace& workspace, const Cell& cell);
double monomial(Vector3 value, MultiIndex index);
double factorial(MultiIndex index);
double binomial(MultiIndex upper, MultiIndex lower);
bool isComponentwiseLessOrEqual(MultiIndex lower, MultiIndex upper);
bool buildHierarchy(const std::vector<Particle>& particles, FmmWorkspace& workspace);
void buildMultipoles(const std::vector<Particle>& particles, FmmWorkspace& workspace);
void evaluateInteractions(const std::vector<Particle>& particles, const ForceLawPolicy& policy,
                          FmmWorkspace& workspace, std::vector<Vector3>& accelerations);

} // namespace bltzr_fmm

#endif // BLITZAR_ENGINE_SRC_PHYSICS_FMMINTERNAL_HPP_
