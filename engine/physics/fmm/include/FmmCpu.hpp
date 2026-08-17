/*
 * @file engine/physics/fmm/include/FmmCpu.hpp
 * @brief Private order-two fast multipole solver contracts.
 */

#ifndef BLITZAR_ENGINE_SRC_PHYSICS_FMMCPU_HPP_
#define BLITZAR_ENGINE_SRC_PHYSICS_FMMCPU_HPP_

#include "ForceLawPolicy.hpp"
#include "Particle.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace bltzr_fmm {

constexpr int kExpansionOrder = 2;
constexpr int kCoefficientCount = 10;

struct Cell final {
    int x = 0;
    int y = 0;
    int z = 0;
    int level = 0;
    int parent = -1;
    int firstParticle = 0;
    int particleCount = 0;
    std::array<int, 8> children{};
    std::array<double, kCoefficientCount> multipole{};
    std::array<double, kCoefficientCount> local{};
};

struct Level final {
    std::vector<Cell> cells;
};

struct Parameters final {
    int leafCapacity = 32;
    int maxDepth = 12;
    float theta = 0.6f;
};

struct Metrics final {
    std::uint32_t leafCount = 0u;
    std::uint32_t minLeafDepth = 0u;
    std::uint32_t maxLeafDepth = 0u;
    std::uint64_t m2lInteractions = 0u;
    std::uint64_t p2pInteractions = 0u;
    bool balancedTwoToOne = false;
    bool finite = false;
};

struct ForceErrorMetrics final {
    double relativeL2 = 0.0;
    double relativeLinf = 0.0;
    double relativeP99 = 0.0;
    bool finite = false;
};

struct StateInvariantMetrics final {
    double totalEnergy = 0.0;
    Vector3 linearMomentum;
    Vector3 angularMomentum;
    bool finite = false;
};

class FmmWorkspace final {
public:
    std::vector<Level> levels;
    std::vector<int> particleOrder;
    std::vector<int> reorderScratch;
    Vector3 origin;
    float sideLength = 1.0f;
    int depth = 0;
    Parameters parameters;
    Metrics metrics;
};

void configure(FmmWorkspace& workspace, int leafCapacity, float theta);
ForceErrorMetrics measureForceError(const std::vector<Vector3>& approximate,
                                    const std::vector<Vector3>& reference);
StateInvariantMetrics measureStateInvariants(const std::vector<Particle>& particles,
                                             const ForceLawPolicy& policy);
bool computeForces(const std::vector<Particle>& particles, const ForceLawPolicy& policy,
                   FmmWorkspace& workspace, std::vector<Vector3>& accelerations);

} // namespace bltzr_fmm

#endif // BLITZAR_ENGINE_SRC_PHYSICS_FMMCPU_HPP_
