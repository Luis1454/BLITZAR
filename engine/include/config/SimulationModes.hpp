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

bool normalizeSolver(std::string_view value, std::string &outCanonical);
bool normalizeIntegrator(std::string_view value, std::string &outCanonical);

} // namespace grav_modes

#endif // SIMULATION_MODES_HPP_
