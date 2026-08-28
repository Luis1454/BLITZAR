#ifndef BLITZAR_INTEGRATION_KDK_KDK_ADVANCE_HPP
#define BLITZAR_INTEGRATION_KDK_KDK_ADVANCE_HPP

#include "integration/kdk/KdkLeapfrog.hpp"

namespace blitzar_integration_kdk {

template <typename ForceProvider>
[[nodiscard]] bool ValidateAdvanceState(const AdvanceState<ForceProvider>& state) noexcept
{
    return state.particles.IsValid() && state.accelerations.IsValid() &&
           state.checkpoint.IsValid() && state.particles.Count() == state.accelerations.Count() &&
           state.particles.Count() == state.checkpoint.Count() && std::isfinite(state.timestep) &&
           state.timestep > 0.0 && state.settings.IsValid() &&
           IsFiniteState(state.particles.State()) &&
           state.solver_particles.count == state.particles.Count() &&
           IsFiniteState(state.solver_particles);
}

template <typename ForceProvider, typename DriftHook, typename RollbackHook>
[[nodiscard]] blitzar_status RollbackAdvance(
    AdvanceRequest<ForceProvider, DriftHook, RollbackHook>& request, blitzar_status status) noexcept
{
    return RestoreWithRollback(
        request.hooks.rollback, request.state.particles, request.state.checkpoint, status);
}

template <typename ForceProvider>
[[nodiscard]] blitzar_status CaptureInitial(AdvanceState<ForceProvider>& state) noexcept
{
    return state.checkpoint.Capture(state.particles.MutableView());
}

template <typename ForceProvider, typename DriftHook, typename RollbackHook>
[[nodiscard]] blitzar_status RunForcePhase(
    AdvanceRequest<ForceProvider, DriftHook, RollbackHook>& request) noexcept
{
    auto& state = request.state;

    const blitzar_core::ForceView force = state.accelerations.View();
    const blitzar_solvers::SolverForceEvaluation force_request{state.solver_particles,
        state.solver_particles, force, state.settings,
        blitzar_solvers::SolverForceSourceKind::Local};

    const blitzar_status status = state.force_provider.Evaluate(force_request);

    if (status != BLITZAR_STATUS_OK) {
        return RollbackAdvance(request, status);
    }
    if (!IsFiniteForce(force)) {
        return RollbackAdvance(request, BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    return BLITZAR_STATUS_OK;
}

template <typename ForceProvider>
void KickAndDrift(AdvanceState<ForceProvider>& state, blitzar_core::ForceView force,
    blitzar_core::Scalar half_step) noexcept
{
    blitzar_core::MutableParticleView mutable_state = state.particles.MutableView();

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t raw_index = 0; raw_index < static_cast<std::int64_t>(state.particles.Count());
        ++raw_index) {
        const std::size_t index = static_cast<std::size_t>(raw_index);

        mutable_state.velocity_x[index] += half_step * force.x[index];
        mutable_state.velocity_y[index] += half_step * force.y[index];
        mutable_state.velocity_z[index] += half_step * force.z[index];
        mutable_state.x[index] += state.timestep * mutable_state.velocity_x[index];
        mutable_state.y[index] += state.timestep * mutable_state.velocity_y[index];
        mutable_state.z[index] += state.timestep * mutable_state.velocity_z[index];
    }
}

template <typename ForceProvider, typename DriftHook, typename RollbackHook>
[[nodiscard]] blitzar_status ApplyDrift(
    AdvanceRequest<ForceProvider, DriftHook, RollbackHook>& request) noexcept
{
    auto& state = request.state;

    const DriftTransition transition =
        request.hooks.drift(state.particles, state.accelerations, state.checkpoint);

    if (transition.status != BLITZAR_STATUS_OK) {
        return RollbackAdvance(request, transition.status);
    }
    if (!transition.state_replaced) {
        return BLITZAR_STATUS_OK;
    }

    const std::size_t checkpoint_count = state.checkpoint.Count();

    if (state.checkpoint.SetCount(state.particles.Count()) != BLITZAR_STATUS_OK) {
        return RollbackAdvance(request, BLITZAR_STATUS_INTERNAL_ERROR);
    }

    if (state.checkpoint.Capture(state.particles.MutableView()) != BLITZAR_STATUS_OK) {
        (void)state.checkpoint.SetCount(checkpoint_count);

        return RollbackAdvance(request, BLITZAR_STATUS_INTERNAL_ERROR);
    }

    state.solver_particles = state.particles.State();

    return BLITZAR_STATUS_OK;
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

        mutable_state.velocity_x[index] += half_step * force.x[index];
        mutable_state.velocity_y[index] += half_step * force.y[index];
        mutable_state.velocity_z[index] += half_step * force.z[index];
    }
}

} // namespace blitzar_integration_kdk

namespace blitzar_integration {

template <typename ForceProvider>
blitzar_status KdkLeapfrog::Advance(
    blitzar_integration_kdk::AdvanceState<ForceProvider>& state) const noexcept
{
    blitzar_integration_kdk::NoopDriftHook drift_hook;
    blitzar_integration_kdk::NoopRollbackHook rollback_hook;
    blitzar_integration_kdk::AdvanceHooks hooks{drift_hook, rollback_hook};

    blitzar_integration_kdk::AdvanceRequest<ForceProvider, decltype(drift_hook),
        decltype(rollback_hook)>
        request{state, hooks};

    return Advance(request);
}

template <typename ForceProvider, typename DriftHook, typename RollbackHook>
blitzar_status KdkLeapfrog::Advance(
    blitzar_integration_kdk::AdvanceRequest<ForceProvider, DriftHook, RollbackHook>& request)
    const noexcept
{
    auto& state = request.state;

    if (!blitzar_integration_kdk::ValidateAdvanceState(state)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    blitzar_status status = blitzar_integration_kdk::CaptureInitial(state);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = blitzar_integration_kdk::RunForcePhase(request);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    const blitzar_core::Scalar half_step = 0.5 * state.timestep;

    blitzar_integration_kdk::KickAndDrift(state, state.accelerations.View(), half_step);

    if (!blitzar_integration_kdk::IsFiniteState(state.particles.State())) {
        return blitzar_integration_kdk::RollbackAdvance(request, BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    status = blitzar_integration_kdk::ApplyDrift(request);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = blitzar_integration_kdk::RunForcePhase(request);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    blitzar_integration_kdk::Kick(state, state.accelerations.View(), half_step);

    return blitzar_integration_kdk::IsFiniteState(state.particles.State())
               ? BLITZAR_STATUS_OK
               : blitzar_integration_kdk::RollbackAdvance(request, BLITZAR_STATUS_INVALID_ARGUMENT);
}

} // namespace blitzar_integration

#endif
