#include "solvers/direct/DirectSolver.hpp"

#include <atomic>
#include <cstdint>

namespace blitzar_direct {

bool DirectSolver::ValidateRequest(
    const blitzar_solvers::SolverForceRequest::Direct& request) const noexcept
{
    return blitzar_core::IsValid(request.targets) && blitzar_core::IsValid(request.sources) &&
           blitzar_core::IsValid(request.forces) && request.targets.count == request.forces.count &&
           request.settings.IsValid() && gravity_.IsValid() && IsValidState(request.targets) &&
           IsValidState(request.sources) && request.range.IsValid(request.sources.SourceCount());
}

blitzar_status DirectSolver::ComputeStaged(
    const blitzar_solvers::SolverForceRequest::Direct& request) noexcept
{
    std::atomic<blitzar_status> status{BLITZAR_STATUS_OK};

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t target_index = 0;
        target_index < static_cast<std::int64_t>(request.targets.count); ++target_index) {
        if (status.load(std::memory_order_relaxed) != BLITZAR_STATUS_OK) {
            continue;
        }

        const std::size_t target = static_cast<std::size_t>(target_index);
        const ForceTargetRequest target_request{gravity_, request, target, staging_[target]};
        const blitzar_status target_status = CalculateTarget(target_request);

        if (target_status != BLITZAR_STATUS_OK) {
            blitzar_status expected = BLITZAR_STATUS_OK;

            status.compare_exchange_strong(
                expected, target_status, std::memory_order_relaxed, std::memory_order_relaxed);
        }
    }

    return status.load(std::memory_order_relaxed);
}

blitzar_status DirectSolver::Commit(
    const blitzar_solvers::SolverForceRequest::Direct& request) noexcept
{
    if (!blitzar_core::IsValid(request.forces) || request.forces.count > staging_.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t target_index = 0;
        target_index < static_cast<std::int64_t>(request.forces.count); ++target_index) {
        const std::size_t target = static_cast<std::size_t>(target_index);

        if (request.range.accumulate) {
            request.forces.x[target] += staging_[target].x;
            request.forces.y[target] += staging_[target].y;
            request.forces.z[target] += staging_[target].z;
        }
        else {
            request.forces.x[target] = staging_[target].x;
            request.forces.y[target] = staging_[target].y;
            request.forces.z[target] = staging_[target].z;
        }
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_direct
