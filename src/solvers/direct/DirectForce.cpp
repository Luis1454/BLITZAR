#include "core/CoreArithmetic.hpp"
#include "physics/reduction/ScalarReduction.hpp"
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
    const blitzar_core::BackendExecutionPolicy& policy = request.evaluation.settings.cpu;
    blitzar_physics::ScalarReduction acceleration_x(policy);
    blitzar_physics::ScalarReduction acceleration_y(policy);
    blitzar_physics::ScalarReduction acceleration_z(policy);

    for (std::size_t source = request.evaluation.range.source_begin;
        source < request.evaluation.range.source_end; ++source) {
        if ((request.evaluation.skip_self && source == request.target) ||
            request.evaluation.sources.mass[source] == 0.0) {
            continue;
        }

        blitzar_core::Vector3 displacement{
            request.evaluation.sources.x[source] - request.evaluation.targets.x[request.target],
            request.evaluation.sources.y[source] - request.evaluation.targets.y[request.target],
            request.evaluation.sources.z[source] - request.evaluation.targets.z[request.target]};

        const blitzar_physics::PeriodicDomain* periodic = request.evaluation.periodic;

        if (periodic != nullptr && periodic->IsEnabled()) {
            displacement = periodic->Fold(displacement);
        }

        const blitzar_core::Scalar distance_squared =
            blitzar_core::MultiplyAdd(displacement.x, displacement.x,
                blitzar_core::MultiplyAdd(displacement.y, displacement.y,
                    blitzar_core::MultiplyAdd(displacement.z, displacement.z, 0.0, policy), policy),
                policy);

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

        acceleration_x.AddProduct(factor, displacement.x);
        acceleration_y.AddProduct(factor, displacement.y);
        acceleration_z.AddProduct(factor, displacement.z);
    }

    request.acceleration = {acceleration_x.Value(), acceleration_y.Value(), acceleration_z.Value()};

    return std::isfinite(request.acceleration.x) && std::isfinite(request.acceleration.y) &&
                   std::isfinite(request.acceleration.z)
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

} // namespace blitzar_direct
