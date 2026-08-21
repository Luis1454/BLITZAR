#ifndef BLITZAR_SOLVERS_DIRECT_DIRECT_SOLVER_HPP
#define BLITZAR_SOLVERS_DIRECT_DIRECT_SOLVER_HPP

#include "core/Solver.hpp"
#include "physics/GravityLaw.hpp"

namespace blitzar_direct {

class DirectSolver final : public blitzar_core::Solver {
public:
    explicit DirectSolver(blitzar_physics::GravityParameters parameters) noexcept;

    [[nodiscard]] blitzar_core::SolverKind Kind() const noexcept override;
    [[nodiscard]] blitzar_status Compute(
        blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings) const noexcept override;

private:
    blitzar_physics::GravityLaw gravity_;
};

}  // namespace blitzar_direct

#endif
