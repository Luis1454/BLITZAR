#include "solvers/pm/PmSolver.hpp"

#include <new>
#include <stdexcept>

namespace blitzar_pm {

PmSolver::PmSolver(blitzar_physics::GravityParameters gravity,
    blitzar_grid::GridResourceConfig grid, std::size_t staging_capacity)
    : gravity_(gravity), resource_(grid), staging_capacity_(staging_capacity), staging_{}
{
    if (staging_capacity_ != 0) {
        staging_.resize(staging_capacity_);
    }
}

blitzar_solvers::SolverKind PmSolver::Kind() const noexcept
{
    return blitzar_solvers::SolverKind::Pm;
}

blitzar_status PmSolver::Prepare(std::size_t staging_capacity) noexcept
{
    return EnsureCapacity(staging_capacity);
}

blitzar_status PmSolver::Evaluate(const blitzar_solvers::SolverForceRequest::Grid& request) noexcept
{
    if (!ValidateRequest(request) || staging_.size() < request.targets.count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status field_status = resource_.BuildField(gravity_);

    if (field_status != BLITZAR_STATUS_OK) {
        return field_status;
    }

    const blitzar_status compute_status = ComputeTargets({request, staging_});

    return compute_status == BLITZAR_STATUS_OK ? Commit(request) : compute_status;
}

blitzar_grid::GridResource& PmSolver::Resource() noexcept
{
    return resource_;
}

const blitzar_grid::GridResource& PmSolver::Resource() const noexcept
{
    return resource_;
}

blitzar_status PmSolver::EnsureCapacity(std::size_t staging_capacity) noexcept
{
    if (staging_capacity <= staging_capacity_) {
        return BLITZAR_STATUS_OK;
    }
    if (staging_capacity > resource_.MaxParticles()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        staging_.resize(staging_capacity);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    staging_capacity_ = staging_capacity;

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_pm
