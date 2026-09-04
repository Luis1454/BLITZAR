#include "solvers/pm/PmSolver.hpp"

#include <atomic>
#include <cmath>
#include <cstdint>

namespace blitzar_pm {

bool PmSolver::IsValidState(blitzar_core::ParticleStateView particles) noexcept
{
    if (!blitzar_core::IsValid(particles)) {
        return false;
    }

    for (std::size_t index = 0; index < particles.SourceCount(); ++index) {
        if (!std::isfinite(particles.x[index]) || !std::isfinite(particles.y[index]) ||
            !std::isfinite(particles.z[index]) || !std::isfinite(particles.velocity_x[index]) ||
            !std::isfinite(particles.velocity_y[index]) ||
            !std::isfinite(particles.velocity_z[index]) || !std::isfinite(particles.mass[index]) ||
            particles.mass[index] < 0.0) {
            return false;
        }
    }

    return true;
}

bool PmSolver::ValidateRequest(
    const blitzar_solvers::SolverForceRequest::Grid& request) const noexcept
{
    return request.source_kind == blitzar_solvers::SolverForceSourceKind::Local &&
           blitzar_core::IsValid(request.targets) && blitzar_core::IsValid(request.sources) &&
           blitzar_core::IsValid(request.forces) && request.targets.count == request.forces.count &&
           request.settings.IsValid() && gravity_.IsValid() && IsValidState(request.targets) &&
           IsValidState(request.sources) && &request.resource == &resource_ &&
           resource_.IsCurrent(request.grid) &&
           request.sources.SourceCount() <= resource_.MaxParticles();
}

blitzar_status PmSolver::ComputeTargets(const ComputeRequest& request) noexcept
{
    std::atomic<blitzar_status> status{BLITZAR_STATUS_OK};

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t target_index = 0;
        target_index < static_cast<std::int64_t>(request.evaluation.targets.count);
        ++target_index) {
        if (status.load(std::memory_order_relaxed) != BLITZAR_STATUS_OK) {
            continue;
        }

        const std::size_t target = static_cast<std::size_t>(target_index);
        const blitzar_core::Vector3 position{request.evaluation.targets.x[target],
            request.evaluation.targets.y[target], request.evaluation.targets.z[target]};

        blitzar_core::Vector3 field{};

        if (!resource_.Interpolate(position, field)) {
            blitzar_status expected = BLITZAR_STATUS_OK;

            status.compare_exchange_strong(expected, BLITZAR_STATUS_INVALID_ARGUMENT,
                std::memory_order_relaxed, std::memory_order_relaxed);

            continue;
        }

        request.staging[target] = field;
    }

    return status.load(std::memory_order_relaxed);
}

blitzar_status PmSolver::Commit(const blitzar_solvers::SolverForceRequest::Grid& request) noexcept
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

        if (request.accumulate) {
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

} // namespace blitzar_pm
