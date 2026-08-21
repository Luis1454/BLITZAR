#include "solvers/direct/DirectSolver.hpp"

#include <cmath>

namespace blitzar_direct {

namespace {

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

    for (std::size_t target = 0; target < particles.count; ++target) {
        blitzar_core::Scalar acceleration_x = 0.0;
        blitzar_core::Scalar acceleration_y = 0.0;
        blitzar_core::Scalar acceleration_z = 0.0;
        for (std::size_t source = 0; source < particles.count; ++source) {
            if (source == target) {
                continue;
            }
            const blitzar_core::Scalar dx = particles.x[source] - particles.x[target];
            const blitzar_core::Scalar dy = particles.y[source] - particles.y[target];
            const blitzar_core::Scalar dz = particles.z[source] - particles.z[target];
            const blitzar_core::Scalar distance_squared = dx * dx + dy * dy + dz * dz;
            if (!gravity_.IsValidPair(particles.mass[source], distance_squared)) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
            const blitzar_core::Scalar factor =
                gravity_.PairFactor(particles.mass[source], distance_squared);
            if (!std::isfinite(factor)) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
            acceleration_x += factor * dx;
            acceleration_y += factor * dy;
            acceleration_z += factor * dz;
        }
        forces.x[target] = acceleration_x;
        forces.y[target] = acceleration_y;
        forces.z[target] = acceleration_z;
    }
    return BLITZAR_STATUS_OK;
}

}  // namespace blitzar_direct
