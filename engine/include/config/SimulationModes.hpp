#ifndef SIMULATION_MODES_HPP_
#define SIMULATION_MODES_HPP_

#include <string>
#include <string_view>

namespace grav_modes {

extern const std::string_view kSolverPairwiseCuda;
extern const std::string_view kSolverOctreeGpu;
extern const std::string_view kSolverOctreeCpu;

extern const std::string_view kIntegratorEuler;
extern const std::string_view kIntegratorRk4;

[[nodiscard]] bool normalizeSolver(std::string_view value, std::string &outCanonical);
[[nodiscard]] bool normalizeIntegrator(std::string_view value, std::string &outCanonical);
[[nodiscard]] bool isSupportedSolverIntegratorPair(std::string_view solver, std::string_view integrator);

} // namespace grav_modes

#endif // SIMULATION_MODES_HPP_
