#ifndef BLITZAR_SOLVERS_DIRECT_DIRECT_SOLVER_HPP
#define BLITZAR_SOLVERS_DIRECT_DIRECT_SOLVER_HPP

#include "core/Execution.hpp"
#include "core/Solver.hpp"
#include "physics/GravityLaw.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <vector>

namespace blitzar_direct {

class DirectSolver final {
public:
    explicit DirectSolver(
        blitzar_physics::GravityParameters parameters, std::size_t staging_capacity = 0);

    [[nodiscard]] blitzar_status Prepare(std::size_t staging_capacity) noexcept;

    [[nodiscard]] blitzar_core::SolverKind Kind() const noexcept;
    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept;
    [[nodiscard]] blitzar_status ComputeRange(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        std::size_t source_begin, std::size_t source_end, bool accumulate) noexcept;

private:
    blitzar_physics::GravityLaw gravity_;
    std::vector<blitzar_core::Vector3> staging_;
};

} // namespace blitzar_direct

#endif
