#include "solvers/direct/DirectSolver.hpp"

#include <atomic>
#include <cmath>
#include <cstdint>

namespace blitzar_direct {

namespace {

struct Acceleration final {
    blitzar_core::Scalar x{};
    blitzar_core::Scalar y{};
    blitzar_core::Scalar z{};
};

[[nodiscard]] bool IsValidState(blitzar_core::ParticleStateView particles) noexcept
{
    for (std::size_t index = 0; index < particles.SourceCount(); ++index) {
        if (!std::isfinite(particles.x[index]) ||
            !std::isfinite(particles.y[index]) ||
            !std::isfinite(particles.z[index]) ||
            !std::isfinite(particles.velocity_x[index]) ||
            !std::isfinite(particles.velocity_y[index]) ||
            !std::isfinite(particles.velocity_z[index]) ||
            !std::isfinite(particles.mass[index]) || particles.mass[index] < 0.0) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] blitzar_status ValidateAndCalculateTarget(
    const blitzar_physics::GravityLaw& gravity,
    std::size_t target,
    blitzar_core::ParticleStateView particles,
    Acceleration& acceleration) noexcept
{
    acceleration = {};
    for (std::size_t source = 0; source < particles.SourceCount(); ++source) {
        if (source == target || particles.mass[source] == 0.0) {
            continue;
        }
        const blitzar_core::Scalar dx = particles.x[source] - particles.x[target];
        const blitzar_core::Scalar dy = particles.y[source] - particles.y[target];
        const blitzar_core::Scalar dz = particles.z[source] - particles.z[target];
        const blitzar_core::Scalar distance_squared = dx * dx + dy * dy + dz * dz;
        const blitzar_physics::PairStatus pair_status =
            gravity.ValidatePair(particles.mass[source], distance_squared);
        if (pair_status != blitzar_physics::PairStatus::Valid) {
            return pair_status == blitzar_physics::PairStatus::Singularity
                       ? BLITZAR_STATUS_SINGULARITY
                       : BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        const blitzar_core::Scalar factor =
            gravity.PairFactor(particles.mass[source], distance_squared);
        if (!std::isfinite(factor)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        acceleration.x += factor * dx;
        acceleration.y += factor * dy;
        acceleration.z += factor * dz;
    }
    if (!std::isfinite(acceleration.x) ||
        !std::isfinite(acceleration.y) ||
        !std::isfinite(acceleration.z)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status CalculateTarget(
    const blitzar_physics::GravityLaw& gravity,
    std::size_t target,
    blitzar_core::ParticleStateView particles,
    Acceleration& acceleration) noexcept
{
    blitzar_core::Scalar acceleration_x = 0.0;
    blitzar_core::Scalar acceleration_y = 0.0;
    blitzar_core::Scalar acceleration_z = 0.0;
#if defined(_OPENMP)
#pragma omp simd reduction(+ : acceleration_x, acceleration_y, acceleration_z)
#endif
    for (std::size_t source = 0; source < particles.SourceCount(); ++source) {
        if (source == target || particles.mass[source] == 0.0) {
            continue;
        }
        const blitzar_core::Scalar dx = particles.x[source] - particles.x[target];
        const blitzar_core::Scalar dy = particles.y[source] - particles.y[target];
        const blitzar_core::Scalar dz = particles.z[source] - particles.z[target];
        const blitzar_core::Scalar distance_squared = dx * dx + dy * dy + dz * dz;
        const blitzar_core::Scalar factor =
            gravity.PairFactor(particles.mass[source], distance_squared);
        acceleration_x += factor * dx;
        acceleration_y += factor * dy;
        acceleration_z += factor * dz;
    }
    acceleration = {acceleration_x, acceleration_y, acceleration_z};
    if (!std::isfinite(acceleration.x) || !std::isfinite(acceleration.y) ||
        !std::isfinite(acceleration.z)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    return BLITZAR_STATUS_OK;
}

}  // namespace

DirectSolver::DirectSolver(blitzar_physics::GravityParameters parameters) noexcept
    : gravity_(parameters)
{
}

blitzar_core::SolverKind DirectSolver::Kind() const noexcept
{
    return blitzar_core::SolverKind::Direct;
}

blitzar_status DirectSolver::Compute(
    blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces,
    const blitzar_core::ExecutionSettings& settings) noexcept
{
    if (!blitzar_core::IsValid(particles) || !blitzar_core::IsValid(forces) ||
        particles.count != forces.count || !settings.IsValid() ||
        !gravity_.IsValid() || !IsValidState(particles)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::atomic<blitzar_status> status{BLITZAR_STATUS_OK};
#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif
    for (std::int64_t target_index = 0;
         target_index < static_cast<std::int64_t>(particles.count);
         ++target_index) {
        if (status.load(std::memory_order_relaxed) != BLITZAR_STATUS_OK) {
            continue;
        }
        const std::size_t target = static_cast<std::size_t>(target_index);
        Acceleration acceleration{};
        const blitzar_status target_status =
            ValidateAndCalculateTarget(
                gravity_, target, particles, acceleration);
        if (target_status != BLITZAR_STATUS_OK) {
            blitzar_status expected = BLITZAR_STATUS_OK;
            status.compare_exchange_strong(
                expected,
                target_status,
                std::memory_order_relaxed,
                std::memory_order_relaxed);
        }
    }
    if (status.load(std::memory_order_relaxed) != BLITZAR_STATUS_OK) {
        return status.load(std::memory_order_relaxed);
    }

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif
    for (std::int64_t target_index = 0;
         target_index < static_cast<std::int64_t>(particles.count);
         ++target_index) {
        if (status.load(std::memory_order_relaxed) != BLITZAR_STATUS_OK) {
            continue;
        }
        const std::size_t target = static_cast<std::size_t>(target_index);
        Acceleration acceleration{};
        const blitzar_status target_status =
            CalculateTarget(gravity_, target, particles, acceleration);
        if (target_status != BLITZAR_STATUS_OK) {
            blitzar_status expected = BLITZAR_STATUS_OK;
            status.compare_exchange_strong(
                expected,
                target_status,
                std::memory_order_relaxed,
                std::memory_order_relaxed);
            continue;
        }
        forces.x[target] = acceleration.x;
        forces.y[target] = acceleration.y;
        forces.z[target] = acceleration.z;
    }
    return status.load(std::memory_order_relaxed);
}

}  // namespace blitzar_direct
