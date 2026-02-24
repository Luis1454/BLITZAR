#ifndef SIMULATION_MODES_HPP_
#define SIMULATION_MODES_HPP_

#include <string>
#include <string_view>

namespace sim::modes {

inline constexpr std::string_view kSolverPairwiseCuda = "pairwise_cuda";
inline constexpr std::string_view kSolverOctreeGpu = "octree_gpu";
inline constexpr std::string_view kSolverOctreeCpu = "octree_cpu";

inline constexpr std::string_view kIntegratorEuler = "euler";
inline constexpr std::string_view kIntegratorRk4 = "rk4";

bool normalizeSolver(std::string_view value, std::string &outCanonical);
bool normalizeIntegrator(std::string_view value, std::string &outCanonical);

} // namespace sim::modes

#endif // SIMULATION_MODES_HPP_
