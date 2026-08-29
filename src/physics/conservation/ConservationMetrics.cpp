#include "physics/conservation/ConservationMetrics.hpp"

#include <cmath>
#include <cstddef>

namespace blitzar_physics {

namespace {

[[nodiscard]] bool IsFiniteState(blitzar_core::ParticleStateView state) noexcept
{
    if (!blitzar_core::IsValid(state)) {
        return false;
    }

    for (std::size_t index = 0; index < state.SourceCount(); ++index) {
        if (!std::isfinite(state.x[index]) || !std::isfinite(state.y[index]) ||
            !std::isfinite(state.z[index]) || !std::isfinite(state.velocity_x[index]) ||
            !std::isfinite(state.velocity_y[index]) || !std::isfinite(state.velocity_z[index]) ||
            !std::isfinite(state.mass[index]) || state.mass[index] < 0.0) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool HasCompleteState(blitzar_core::ParticleStateView state) noexcept
{
    return state.SourceCount() == state.count;
}

[[nodiscard]] PairStatus ValidateSource(const GravityLaw& gravity, blitzar_core::Scalar source_mass,
    blitzar_core::Scalar squared_distance) noexcept
{
    return gravity.ValidatePair(source_mass, squared_distance);
}

[[nodiscard]] blitzar_status ToStatus(PairStatus status) noexcept
{
    return status == PairStatus::Singularity ? BLITZAR_STATUS_SINGULARITY
           : status == PairStatus::Invalid   ? BLITZAR_STATUS_INVALID_ARGUMENT
                                             : BLITZAR_STATUS_OK;
}

struct PotentialContext final {
    const GravityLaw& gravity;
    blitzar_core::Scalar effective_constant{};
    blitzar_core::Scalar effective_softening{};
};

struct MetricAccumulator final {
    ScalarReduction kinetic;
    ScalarReduction potential;
    ScalarReduction momentum_x;
    ScalarReduction momentum_y;
    ScalarReduction momentum_z;
};

[[nodiscard]] bool IsValidReduction(ReductionKind reduction) noexcept
{
    return reduction == ReductionKind::Plain || reduction == ReductionKind::Kahan ||
           reduction == ReductionKind::Neumaier;
}

[[nodiscard]] bool IsFinite(const MetricAccumulator& accumulator) noexcept
{
    return std::isfinite(accumulator.kinetic.Value()) &&
           std::isfinite(accumulator.potential.Value()) &&
           std::isfinite(accumulator.momentum_x.Value()) &&
           std::isfinite(accumulator.momentum_y.Value()) &&
           std::isfinite(accumulator.momentum_z.Value());
}

[[nodiscard]] PairStatus ValidateSources(const PotentialContext& context,
    blitzar_core::Scalar first_mass, blitzar_core::Scalar second_mass,
    blitzar_core::Scalar squared_distance) noexcept
{
    const PairStatus first_status = ValidateSource(context.gravity, first_mass, squared_distance);

    return first_status != PairStatus::Valid
               ? first_status
               : ValidateSource(context.gravity, second_mass, squared_distance);
}

[[nodiscard]] blitzar_status AccumulateKinetic(
    blitzar_core::ParticleStateView state, MetricAccumulator& accumulator) noexcept
{
    for (std::size_t index = 0; index < state.count; ++index) {
        const blitzar_core::Scalar mass = state.mass[index];
        const blitzar_core::Scalar velocity_x = state.velocity_x[index];
        const blitzar_core::Scalar velocity_y = state.velocity_y[index];
        const blitzar_core::Scalar velocity_z = state.velocity_z[index];
        const blitzar_core::Scalar speed_squared =
            velocity_x * velocity_x + velocity_y * velocity_y + velocity_z * velocity_z;

        const blitzar_core::Scalar kinetic = 0.5 * mass * speed_squared;
        const blitzar_core::Scalar momentum_x = mass * velocity_x;
        const blitzar_core::Scalar momentum_y = mass * velocity_y;
        const blitzar_core::Scalar momentum_z = mass * velocity_z;

        if (!std::isfinite(speed_squared)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        if (!std::isfinite(kinetic)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        if (!std::isfinite(momentum_x) || !std::isfinite(momentum_y) ||
            !std::isfinite(momentum_z)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        accumulator.momentum_x.Add(momentum_x);
        accumulator.momentum_y.Add(momentum_y);
        accumulator.momentum_z.Add(momentum_z);
        accumulator.kinetic.Add(kinetic);

        if (!IsFinite(accumulator)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status AccumulatePotential(blitzar_core::ParticleStateView state,
    const PotentialContext& context, MetricAccumulator& accumulator) noexcept
{
    for (std::size_t first = 0; first < state.count; ++first) {
        for (std::size_t second = first + 1; second < state.count; ++second) {
            if (state.mass[first] == 0.0 || state.mass[second] == 0.0) {
                continue;
            }

            const blitzar_core::Scalar dx = state.x[second] - state.x[first];
            const blitzar_core::Scalar dy = state.y[second] - state.y[first];
            const blitzar_core::Scalar dz = state.z[second] - state.z[first];
            const blitzar_core::Scalar squared_distance = dx * dx + dy * dy + dz * dz;
            const PairStatus pair_status =
                ValidateSources(context, state.mass[first], state.mass[second], squared_distance);

            if (pair_status != PairStatus::Valid) {
                return ToStatus(pair_status);
            }

            const blitzar_core::Scalar softened_squared_distance =
                squared_distance + context.effective_softening * context.effective_softening;

            const blitzar_core::Scalar softened_distance = std::sqrt(softened_squared_distance);
            const blitzar_core::Scalar mass_product = state.mass[first] * state.mass[second];
            const blitzar_core::Scalar potential =
                -context.effective_constant * mass_product / softened_distance;

            if (!std::isfinite(softened_squared_distance)) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
            if (!std::isfinite(softened_distance)) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
            if (!std::isfinite(mass_product) || !std::isfinite(potential)) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            accumulator.potential.Add(potential);

            if (!IsFinite(accumulator)) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
        }
    }

    return BLITZAR_STATUS_OK;
}

} // namespace

bool ConservationMetrics::IsFinite() const noexcept
{
    return std::isfinite(kinetic_energy) && std::isfinite(potential_energy) &&
           std::isfinite(total_energy) && std::isfinite(momentum.x) && std::isfinite(momentum.y) &&
           std::isfinite(momentum.z);
}

blitzar_status ComputeConservationMetrics(blitzar_core::ParticleStateView state,
    GravityParameters parameters, ConservationMetrics& metrics) noexcept
{
    return ComputeConservationMetrics(state, parameters, ReductionKind::Neumaier, metrics);
}

blitzar_status ComputeConservationMetrics(blitzar_core::ParticleStateView state,
    GravityParameters parameters, ReductionKind reduction, ConservationMetrics& metrics) noexcept
{
    if (!IsValidReduction(reduction) || !HasCompleteState(state) || !IsFiniteState(state) ||
        !parameters.IsValid()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const GravityLaw gravity(parameters);
    ConservationMetrics candidate{};
    MetricAccumulator accumulator{ScalarReduction(reduction), ScalarReduction(reduction),
        ScalarReduction(reduction), ScalarReduction(reduction), ScalarReduction(reduction)};

    const PotentialContext context{
        gravity, parameters.EffectiveConstant(), parameters.EffectiveSoftening()};

    if (!gravity.IsValid() || !std::isfinite(context.effective_constant) ||
        !std::isfinite(context.effective_softening)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status kinetic_status = AccumulateKinetic(state, accumulator);

    if (kinetic_status != BLITZAR_STATUS_OK) {
        return kinetic_status;
    }

    const blitzar_status potential_status = AccumulatePotential(state, context, accumulator);

    if (potential_status != BLITZAR_STATUS_OK) {
        return potential_status;
    }

    candidate.kinetic_energy = accumulator.kinetic.Value();
    candidate.potential_energy = accumulator.potential.Value();
    candidate.momentum = {accumulator.momentum_x.Value(), accumulator.momentum_y.Value(),
        accumulator.momentum_z.Value()};

    ScalarReduction total_energy(reduction);

    total_energy.Add(candidate.kinetic_energy);
    total_energy.Add(candidate.potential_energy);

    candidate.total_energy = total_energy.Value();

    if (!candidate.IsFinite()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    metrics = candidate;

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_physics
