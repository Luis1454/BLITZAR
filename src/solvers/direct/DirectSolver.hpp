#ifndef BLITZAR_SOLVERS_DIRECT_DIRECT_SOLVER_HPP
#define BLITZAR_SOLVERS_DIRECT_DIRECT_SOLVER_HPP

#include "core/Solver.hpp"
#include "core/Execution.hpp"
#include "physics/GravityLaw.hpp"

#include <blitzar/blitzar.h>

namespace blitzar_direct {

class DirectSolver final {
public:
    explicit DirectSolver(blitzar_physics::GravityParameters parameters) noexcept;

    [[nodiscard]] blitzar_core::SolverKind Kind() const noexcept;
    [[nodiscard]] blitzar_status Compute(
        blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings) noexcept;

private:
    blitzar_physics::GravityLaw gravity_;
};

}  // namespace blitzar_direct

#endif
