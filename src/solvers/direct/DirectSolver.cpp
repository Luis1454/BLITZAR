#include "solvers/direct/DirectSolver.hpp"

#include <new>
#include <stdexcept>

namespace blitzar_direct {

DirectSolver::DirectSolver(
    blitzar_physics::GravityParameters parameters, std::size_t staging_capacity)
    : gravity_(parameters), staging_{}
{
    if (staging_capacity != 0) {
        staging_.resize(staging_capacity);
    }
}

blitzar_status DirectSolver::Prepare(std::size_t staging_capacity) noexcept
{
    try {
        if (staging_.size() < staging_capacity) {
            staging_.resize(staging_capacity);
        }
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_solvers::SolverKind DirectSolver::Kind() const noexcept
{
    return blitzar_solvers::SolverKind::Direct;
}

blitzar_status DirectSolver::Evaluate(
    const blitzar_solvers::SolverForceRequest::Direct& request) noexcept
{
    if (!ValidateRequest(request) || staging_.size() < request.targets.count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status staged_status = ComputeStaged(request);

    return staged_status == BLITZAR_STATUS_OK ? Commit(request) : staged_status;
}

} // namespace blitzar_direct
