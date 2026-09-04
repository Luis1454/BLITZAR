#ifndef BLITZAR_SOLVERS_PM_PM_SOLVER_HPP
#define BLITZAR_SOLVERS_PM_PM_SOLVER_HPP

#include "grid/GridResource.hpp"
#include "physics/gravity/GravityLaw.hpp"
#include "solvers/SolverContract.hpp"
#include "solvers/SolverForceRequest.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <vector>

namespace blitzar_pm {

class PmSolver final {
public:
    PmSolver(blitzar_physics::GravityParameters gravity, blitzar_grid::GridResourceConfig grid,
        std::size_t staging_capacity);

    PmSolver(const PmSolver&) = delete;
    PmSolver& operator=(const PmSolver&) = delete;
    PmSolver(PmSolver&&) noexcept = default;
    PmSolver& operator=(PmSolver&&) noexcept = default;

    [[nodiscard]] blitzar_solvers::SolverKind Kind() const noexcept;
    [[nodiscard]] blitzar_status Prepare(std::size_t staging_capacity) noexcept;
    [[nodiscard]] blitzar_status Evaluate(
        const blitzar_solvers::SolverForceRequest::Grid& request) noexcept;
    [[nodiscard]] blitzar_grid::GridResource& Resource() noexcept;
    [[nodiscard]] const blitzar_grid::GridResource& Resource() const noexcept;

private:
    struct ComputeRequest final {
        const blitzar_solvers::SolverForceRequest::Grid& evaluation;
        std::vector<blitzar_core::Vector3>& staging;
    };

    [[nodiscard]] static bool IsValidState(blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] bool ValidateRequest(
        const blitzar_solvers::SolverForceRequest::Grid& request) const noexcept;
    [[nodiscard]] blitzar_status ComputeTargets(const ComputeRequest& request) noexcept;
    [[nodiscard]] blitzar_status Commit(
        const blitzar_solvers::SolverForceRequest::Grid& request) noexcept;
    [[nodiscard]] blitzar_status EnsureCapacity(std::size_t staging_capacity) noexcept;

    blitzar_physics::GravityLaw gravity_;
    blitzar_grid::GridResource resource_;
    std::size_t staging_capacity_{};
    std::vector<blitzar_core::Vector3> staging_{};
};

} // namespace blitzar_pm

#endif
