#ifndef BLITZAR_INTEGRATION_COMOVING_COMOVING_ADVANCE_HPP
#define BLITZAR_INTEGRATION_COMOVING_COMOVING_ADVANCE_HPP

#include "core/CoreArithmetic.hpp"
#include "integration/comoving/ComovingLeapfrog.hpp"
#include "integration/kdk/KdkLeapfrog.hpp"

namespace blitzar_integration_comoving {

template <typename ForceProvider>
[[nodiscard]] blitzar_status RollbackAdvance(
    AdvanceState<ForceProvider>& state, blitzar_status status) noexcept
{
    blitzar_integration_kdk::NoopRollbackHook rollback_hook;

    return blitzar_integration_kdk::RestoreWithRollback(
        rollback_hook, state.particles, state.checkpoint, status);
}

template <typename ForceProvider>
[[nodiscard]] bool ValidateAdvanceState(const AdvanceState<ForceProvider>& state) noexcept
{
    const bool valid_expansion =
        state.background.IsValid() &&
        (state.background.kind == blitzar_integration::ExpansionKind::Static ||
            state.cosmic_time > 0.0);

    return valid_expansion && state.particles.IsValid() && state.accelerations.IsValid() &&
           state.checkpoint.IsValid() && state.particles.Count() == state.accelerations.Count() &&
           state.particles.Count() == state.checkpoint.Count() &&
           std::isfinite(state.cosmic_time) && std::isfinite(state.timestep) &&
           state.timestep > 0.0 && state.settings.IsValid() &&
           blitzar_integration_kdk::IsFiniteState(state.particles.State()) &&
           state.solver_particles.count == state.particles.Count() &&
           blitzar_integration_kdk::IsFiniteState(state.solver_particles);
}

template <typename ForceProvider>
[[nodiscard]] blitzar_status CaptureInitial(AdvanceState<ForceProvider>& state) noexcept
{
    return state.checkpoint.Capture(state.particles.MutableView());
}

template <typename ForceProvider>
void Kick(AdvanceState<ForceProvider>& state, blitzar_core::ForceView force,
    blitzar_core::Scalar half_step) noexcept
{
    blitzar_core::MutableParticleView mutable_state = state.particles.MutableView();

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t raw_index = 0; raw_index < static_cast<std::int64_t>(state.particles.Count());
        ++raw_index) {
        const std::size_t index = static_cast<std::size_t>(raw_index);

        mutable_state.velocity_x[index] = blitzar_core::MultiplyAdd(
            half_step, force.x[index], mutable_state.velocity_x[index], state.settings.cpu);

        mutable_state.velocity_y[index] = blitzar_core::MultiplyAdd(
            half_step, force.y[index], mutable_state.velocity_y[index], state.settings.cpu);

        mutable_state.velocity_z[index] = blitzar_core::MultiplyAdd(
            half_step, force.z[index], mutable_state.velocity_z[index], state.settings.cpu);
    }
}

template <typename ForceProvider>
[[nodiscard]] blitzar_status RunForcePhase(AdvanceState<ForceProvider>& state) noexcept
{
    const blitzar_core::ForceView force = state.accelerations.View();
    const blitzar_solvers::SolverForceEvaluation force_request{state.solver_particles,
        state.solver_particles, force, state.settings,
        blitzar_solvers::SolverForceSourceKind::Local};

    const blitzar_status status = state.force_provider.Evaluate(force_request);

    if (status != BLITZAR_STATUS_OK) {
        return RollbackAdvance(state, status);
    }
    if (!blitzar_integration_kdk::IsFiniteForce(force)) {
        return RollbackAdvance(state, BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    return BLITZAR_STATUS_OK;
}

template <typename ForceProvider>
[[nodiscard]] blitzar_status Drift(AdvanceState<ForceProvider>& state) noexcept
{
    const blitzar_core::Scalar drift_length =
        blitzar_integration::ComovingDrift(state.background, state.cosmic_time, state.timestep);

    blitzar_core::MutableParticleView mutable_state = state.particles.MutableView();

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t raw_index = 0; raw_index < static_cast<std::int64_t>(state.particles.Count());
        ++raw_index) {
        const std::size_t index = static_cast<std::size_t>(raw_index);

        mutable_state.x[index] = blitzar_core::MultiplyAdd(drift_length,
            mutable_state.velocity_x[index], mutable_state.x[index], state.settings.cpu);

        mutable_state.y[index] = blitzar_core::MultiplyAdd(drift_length,
            mutable_state.velocity_y[index], mutable_state.y[index], state.settings.cpu);

        mutable_state.z[index] = blitzar_core::MultiplyAdd(drift_length,
            mutable_state.velocity_z[index], mutable_state.z[index], state.settings.cpu);
    }

    state.solver_particles = state.particles.State();

    return blitzar_integration_kdk::IsFiniteState(state.particles.State())
               ? BLITZAR_STATUS_OK
               : RollbackAdvance(state, BLITZAR_STATUS_INVALID_ARGUMENT);
}

} // namespace blitzar_integration_comoving

namespace blitzar_integration {

template <typename ForceProvider>
blitzar_status ComovingLeapfrog::Advance(
    blitzar_integration_comoving::AdvanceState<ForceProvider>& state) const noexcept
{
    if (!blitzar_integration_comoving::ValidateAdvanceState(state)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    blitzar_status status = blitzar_integration_comoving::CaptureInitial(state);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = blitzar_integration_comoving::RunForcePhase(state);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    const blitzar_core::Scalar half_step = 0.5 * state.timestep;

    blitzar_integration_comoving::Kick(state, state.accelerations.View(), half_step);

    status = blitzar_integration_comoving::Drift(state);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = blitzar_integration_comoving::RunForcePhase(state);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    blitzar_integration_comoving::Kick(state, state.accelerations.View(), half_step);

    state.cosmic_time += state.timestep;

    return blitzar_integration_kdk::IsFiniteState(state.particles.State())
               ? BLITZAR_STATUS_OK
               : blitzar_integration_comoving::RollbackAdvance(
                     state, BLITZAR_STATUS_INVALID_ARGUMENT);
}

} // namespace blitzar_integration

#endif
