#include "solvers/direct/DirectSolver.hpp"

#include <atomic>
#include <cstdint>

namespace blitzar_direct {

bool DirectSolver::ValidateRangeRequest(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
    blitzar_core::ForceRange range) const noexcept
{
    return blitzar_core::IsValid(particles) && blitzar_core::IsValid(forces) &&
           particles.count == forces.count && settings.IsValid() && gravity_.IsValid() &&
           IsValidState(particles) && range.IsValid(particles.SourceCount());
}

bool DirectSolver::ValidateRemoteRequest(blitzar_core::ParticleStateView targets,
    blitzar_core::ParticleStateView sources, blitzar_core::ForceView forces,
    const blitzar_core::ExecutionSettings& settings) const noexcept
{
    return blitzar_core::IsValid(targets) && blitzar_core::IsValid(sources) &&
           blitzar_core::IsValid(forces) && targets.count == forces.count && settings.IsValid() &&
           gravity_.IsValid() && IsValidState(targets) && IsValidState(sources);
}

blitzar_status DirectSolver::ComputeRangeStaged(
    blitzar_core::ParticleStateView particles, blitzar_core::ForceRange range) noexcept
{
    std::atomic<blitzar_status> status{BLITZAR_STATUS_OK};

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t target_index = 0; target_index < static_cast<std::int64_t>(particles.count);
         ++target_index) {
        if (status.load(std::memory_order_relaxed) != BLITZAR_STATUS_OK) {
            continue;
        }

        const std::size_t target = static_cast<std::size_t>(target_index);
        const ForceTargetRequest request{gravity_, target, particles, range, staging_[target]};
        const blitzar_status target_status = CalculateTarget(request);

        if (target_status != BLITZAR_STATUS_OK) {
            blitzar_status expected = BLITZAR_STATUS_OK;

            status.compare_exchange_strong(
                expected, target_status, std::memory_order_relaxed, std::memory_order_relaxed);
        }
    }

    return status.load(std::memory_order_relaxed);
}

blitzar_status DirectSolver::CommitRange(
    blitzar_core::ForceView forces, blitzar_core::ForceRange range) noexcept
{
    if (!blitzar_core::IsValid(forces) || forces.count > staging_.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t target_index = 0; target_index < static_cast<std::int64_t>(forces.count);
         ++target_index) {
        const std::size_t target = static_cast<std::size_t>(target_index);

        if (range.accumulate) {
            forces.x[target] += staging_[target].x;
            forces.y[target] += staging_[target].y;
            forces.z[target] += staging_[target].z;
        }
        else {
            forces.x[target] = staging_[target].x;
            forces.y[target] = staging_[target].y;
            forces.z[target] = staging_[target].z;
        }
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status DirectSolver::ComputeRemoteStaged(blitzar_core::ParticleStateView targets,
    blitzar_core::ParticleStateView sources) noexcept
{
    std::atomic<blitzar_status> status{BLITZAR_STATUS_OK};

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t target_index = 0; target_index < static_cast<std::int64_t>(targets.count);
         ++target_index) {
        if (status.load(std::memory_order_relaxed) != BLITZAR_STATUS_OK) {
            continue;
        }

        const std::size_t target = static_cast<std::size_t>(target_index);
        const RemoteForceTargetRequest request{
            gravity_, targets, sources, target, staging_[target]};

        const blitzar_status target_status = CalculateRemoteTarget(request);

        if (target_status != BLITZAR_STATUS_OK) {
            blitzar_status expected = BLITZAR_STATUS_OK;

            status.compare_exchange_strong(
                expected, target_status, std::memory_order_relaxed, std::memory_order_relaxed);
        }
    }

    return status.load(std::memory_order_relaxed);
}

blitzar_status DirectSolver::CommitRemote(blitzar_core::ForceView forces) noexcept
{
    if (!blitzar_core::IsValid(forces) || forces.count > staging_.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t target_index = 0; target_index < static_cast<std::int64_t>(forces.count);
         ++target_index) {
        const std::size_t target = static_cast<std::size_t>(target_index);

        forces.x[target] += staging_[target].x;
        forces.y[target] += staging_[target].y;
        forces.z[target] += staging_[target].z;
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_direct
