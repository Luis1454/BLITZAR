/*
 * @file engine/include/config/SimulationModes.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONMODES_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONMODES_HPP_
#include <string>
#include <string_view>

namespace bltzr_modes {
extern const std::string_view kSolverPairwiseCuda;
extern const std::string_view kSolverOctreeGpu;
extern const std::string_view kSolverOctreeCpu;
extern const std::string_view kIntegratorEuler;
extern const std::string_view kIntegratorRk4;
extern const std::string_view kIntegratorLeapfrog;
extern const std::string_view kOctreeCriterionCom;
extern const std::string_view kOctreeCriterionBounds;
[[nodiscard]] bool normalizeSolver(std::string_view value, std::string& outCanonical);
[[nodiscard]] bool normalizeIntegrator(std::string_view value, std::string& outCanonical);
[[nodiscard]] bool normalizeOctreeOpeningCriterion(std::string_view value,
                                                   std::string& outCanonical);
[[nodiscard]] bool isSupportedSolverIntegratorPair(std::string_view solver,
                                                   std::string_view integrator);
} // namespace bltzr_modes
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_SIMULATIONMODES_HPP_
