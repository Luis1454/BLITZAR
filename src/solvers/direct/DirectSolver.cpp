#include "solvers/direct/DirectSolver.hpp"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <new>
#include <stdexcept>

namespace blitzar_direct {

namespace {

struct ForceTargetRequest final {
    const blitzar_physics::GravityLaw& gravity;
    std::size_t target{};
    blitzar_core::ParticleStateView particles;
    blitzar_core::ForceRange range;
    blitzar_core::Vector3& acceleration;
};

[[nodiscard]] bool IsValidState(blitzar_core::ParticleStateView particles) noexcept
{
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

[[nodiscard]] blitzar_status CalculateTarget(const ForceTargetRequest& request) noexcept
{
    blitzar_core::Scalar acceleration_x = 0.0;
    blitzar_core::Scalar acceleration_y = 0.0;
    blitzar_core::Scalar acceleration_z = 0.0;

    for (std::size_t source = request.range.source_begin; source < request.range.source_end;
         ++source) {
        if (source == request.target || request.particles.mass[source] == 0.0) {
            continue;
        }

        const blitzar_core::Scalar dx =
            request.particles.x[source] - request.particles.x[request.target];

        const blitzar_core::Scalar dy =
            request.particles.y[source] - request.particles.y[request.target];

        const blitzar_core::Scalar dz =
            request.particles.z[source] - request.particles.z[request.target];

        const blitzar_core::Scalar distance_squared = dx * dx + dy * dy + dz * dz;
        const blitzar_physics::PairStatus pair_status =
            request.gravity.ValidatePair(request.particles.mass[source], distance_squared);

        if (pair_status != blitzar_physics::PairStatus::Valid) {
            return pair_status == blitzar_physics::PairStatus::Singularity
                       ? BLITZAR_STATUS_SINGULARITY
                       : BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const blitzar_core::Scalar factor =
            request.gravity.PairFactor(request.particles.mass[source], distance_squared);

        if (!std::isfinite(factor)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        acceleration_x += factor * dx;
        acceleration_y += factor * dy;
        acceleration_z += factor * dz;
    }

    request.acceleration = {acceleration_x, acceleration_y, acceleration_z};

    if (!std::isfinite(request.acceleration.x) || !std::isfinite(request.acceleration.y) ||
        !std::isfinite(request.acceleration.z)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

} // namespace

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

blitzar_core::SolverKind DirectSolver::Kind() const noexcept
{
    return blitzar_core::SolverKind::Direct;
}

blitzar_status DirectSolver::Compute(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept
{
    return ComputeRange(particles, forces, settings, {0, particles.SourceCount(), false});
}

blitzar_status DirectSolver::ComputeRange(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
    blitzar_core::ForceRange range) noexcept
{
    if (!blitzar_core::IsValid(particles) || !blitzar_core::IsValid(forces) ||
        particles.count != forces.count || !settings.IsValid() || !gravity_.IsValid() ||
        !IsValidState(particles) || !range.IsValid(particles.SourceCount())) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (staging_.size() < particles.count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

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

    if (status.load(std::memory_order_relaxed) != BLITZAR_STATUS_OK) {
        return status.load(std::memory_order_relaxed);
    }

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t target_index = 0; target_index < static_cast<std::int64_t>(particles.count);
         ++target_index) {
        const std::size_t target = static_cast<std::size_t>(target_index);
        const blitzar_core::Vector3& acceleration = staging_[target];

        if (range.accumulate) {
            forces.x[target] += acceleration.x;
            forces.y[target] += acceleration.y;
            forces.z[target] += acceleration.z;
        }
        else {
            forces.x[target] = acceleration.x;
            forces.y[target] = acceleration.y;
            forces.z[target] = acceleration.z;
        }
    }

    return status.load(std::memory_order_relaxed);
}

} // namespace blitzar_direct
