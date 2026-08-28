#include "solvers/direct/DirectSolver.hpp"

#include <cmath>

namespace blitzar_direct {

bool DirectSolver::IsValidState(blitzar_core::ParticleStateView particles) noexcept
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

blitzar_status DirectSolver::CalculateTarget(const ForceTargetRequest& request) noexcept
{
    blitzar_core::Scalar acceleration_x = 0.0;
    blitzar_core::Scalar acceleration_y = 0.0;
    blitzar_core::Scalar acceleration_z = 0.0;

    for (std::size_t source = request.evaluation.range.source_begin;
        source < request.evaluation.range.source_end; ++source) {
        if ((request.evaluation.skip_self && source == request.target) ||
            request.evaluation.sources.mass[source] == 0.0) {
            continue;
        }

        const blitzar_core::Scalar dx =
            request.evaluation.sources.x[source] - request.evaluation.targets.x[request.target];

        const blitzar_core::Scalar dy =
            request.evaluation.sources.y[source] - request.evaluation.targets.y[request.target];

        const blitzar_core::Scalar dz =
            request.evaluation.sources.z[source] - request.evaluation.targets.z[request.target];

        const blitzar_core::Scalar distance_squared = dx * dx + dy * dy + dz * dz;
        const blitzar_physics::PairStatus pair_status =
            request.gravity.ValidatePair(request.evaluation.sources.mass[source], distance_squared);

        if (pair_status != blitzar_physics::PairStatus::Valid) {
            return pair_status == blitzar_physics::PairStatus::Singularity
                       ? BLITZAR_STATUS_SINGULARITY
                       : BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const blitzar_core::Scalar factor =
            request.gravity.PairFactor(request.evaluation.sources.mass[source], distance_squared);

        if (!std::isfinite(factor)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        acceleration_x += factor * dx;
        acceleration_y += factor * dy;
        acceleration_z += factor * dz;
    }

    request.acceleration = {acceleration_x, acceleration_y, acceleration_z};

    return std::isfinite(acceleration_x) && std::isfinite(acceleration_y) &&
                   std::isfinite(acceleration_z)
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

} // namespace blitzar_direct
