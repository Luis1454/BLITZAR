#ifndef BLITZAR_SOLVERS_DIRECT_DIRECT_SOLVER_HPP
#define BLITZAR_SOLVERS_DIRECT_DIRECT_SOLVER_HPP

#include "core/CoreExecution.hpp"
#include "physics/gravity/GravityLaw.hpp"
#include "solvers/SolverContract.hpp"
#include "solvers/SolverForceRequest.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <vector>

namespace blitzar_direct {

class DirectSolver final {
public:
    explicit DirectSolver(
        blitzar_physics::GravityParameters parameters, std::size_t staging_capacity = 0);

    [[nodiscard]] blitzar_status Prepare(std::size_t staging_capacity) noexcept;

    [[nodiscard]] blitzar_solvers::SolverKind Kind() const noexcept;
    [[nodiscard]] blitzar_status Evaluate(
        const blitzar_solvers::SolverForceRequest::Direct& request) noexcept;

private:
    struct ForceTargetRequest final {
        const blitzar_physics::GravityLaw& gravity;
        const blitzar_solvers::SolverForceRequest::Direct& evaluation;
        std::size_t target{};
        blitzar_core::Vector3& acceleration;
    };

    [[nodiscard]] static bool IsValidState(blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] static blitzar_status CalculateTarget(const ForceTargetRequest& request) noexcept;
    [[nodiscard]] bool ValidateRequest(
        const blitzar_solvers::SolverForceRequest::Direct& request) const noexcept;
    [[nodiscard]] blitzar_status ComputeStaged(
        const blitzar_solvers::SolverForceRequest::Direct& request) noexcept;
    [[nodiscard]] blitzar_status Commit(
        const blitzar_solvers::SolverForceRequest::Direct& request) noexcept;

    blitzar_physics::GravityLaw gravity_;
    std::vector<blitzar_core::Vector3> staging_;
};

} // namespace blitzar_direct

#endif
