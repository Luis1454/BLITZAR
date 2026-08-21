#include "solvers/direct/DirectSolver.hpp"

#include <cmath>

namespace blitzar_direct {

namespace {

struct Acceleration final {
    blitzar_core::Scalar x{};
    blitzar_core::Scalar y{};
    blitzar_core::Scalar z{};
};

[[nodiscard]] bool IsValidState(blitzar_core::ParticleStateView particles) noexcept
{
    for (std::size_t index = 0; index < particles.count; ++index) {
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

[[nodiscard]] blitzar_status CalculateTarget(
    const blitzar_physics::GravityLaw& gravity,
    std::size_t target,
    blitzar_core::ParticleStateView particles,
    Acceleration& acceleration) noexcept
{
    acceleration = {};
    for (std::size_t source = 0; source < particles.count; ++source) {
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

    // Validate every target before committing any force output.
    for (std::size_t target = 0; target < particles.count; ++target) {
        Acceleration acceleration{};
        const blitzar_status status =
            CalculateTarget(gravity_, target, particles, acceleration);
        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
    }
    for (std::size_t target = 0; target < particles.count; ++target) {
        Acceleration acceleration{};
        const blitzar_status status =
            CalculateTarget(gravity_, target, particles, acceleration);
        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
        forces.x[target] = acceleration.x;
        forces.y[target] = acceleration.y;
        forces.z[target] = acceleration.z;
    }
    return BLITZAR_STATUS_OK;
}

}  // namespace blitzar_direct
