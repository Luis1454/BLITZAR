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

blitzar_status DirectSolver::Compute(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept
{
    return ComputeRange(particles, forces, settings, {0, particles.SourceCount(), false});
}

blitzar_status DirectSolver::ComputeRange(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
    blitzar_solvers::ForceRange range) noexcept
{
    if (!ValidateRangeRequest(particles, forces, settings, range) ||
        staging_.size() < particles.count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status staged_status = ComputeRangeStaged(particles, range);

    return staged_status == BLITZAR_STATUS_OK ? CommitRange(forces, range) : staged_status;
}

blitzar_status DirectSolver::ComputeRemote(blitzar_core::ParticleStateView targets,
    blitzar_core::ParticleStateView sources, blitzar_core::ForceView forces,
    const blitzar_core::ExecutionSettings& settings) noexcept
{
    if (!ValidateRemoteRequest(targets, sources, forces, settings) ||
        staging_.size() < targets.count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status staged_status = ComputeRemoteStaged(targets, sources);

    return staged_status == BLITZAR_STATUS_OK ? CommitRemote(forces) : staged_status;
}

} // namespace blitzar_direct
