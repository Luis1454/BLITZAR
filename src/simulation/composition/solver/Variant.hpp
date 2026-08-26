#ifndef BLITZAR_SIMULATION_COMPOSITION_SOLVER_VARIANT_HPP
#define BLITZAR_SIMULATION_COMPOSITION_SOLVER_VARIANT_HPP

#include "solvers/barnes_hut/BarnesHutSolver.hpp"
#include "solvers/direct/DirectSolver.hpp"
#include "solvers/fmm/FmmSolver.hpp"

#include <variant>

namespace blitzar_sim {

using SolverVariant = std::variant<blitzar_direct::DirectSolver,
    blitzar_barnes_hut::BarnesHutSolver, blitzar_fmm::FmmSolver>;

} // namespace blitzar_sim

#endif
