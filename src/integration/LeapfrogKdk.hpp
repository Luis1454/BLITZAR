#ifndef BLITZAR_INTEGRATION_LEAPFROG_KDK_HPP
#define BLITZAR_INTEGRATION_LEAPFROG_KDK_HPP

#include "core/Execution.hpp"
#include "integration/LeapfrogWorkspace.hpp"
#include "particles/ParticleBuffer.hpp"

#include <cmath>
#include <concepts>
#include <cstdint>
#include <span>

namespace blitzar_integration_kdk {

struct DriftTransition final {
    blitzar_status status{BLITZAR_STATUS_OK};
    bool state_replaced{false};
};

struct NoopDriftHook final {
    [[nodiscard]] DriftTransition operator()(blitzar_particles::ParticleBuffer&,
        blitzar_particles::AccelerationBuffer&,
        blitzar_integration::LeapfrogWorkspace&) const noexcept
    {
        return {};
    }
};

struct NoopRollbackHook final {
    void operator()() const noexcept {}
};

[[nodiscard]] inline bool IsFiniteState(blitzar_core::ParticleStateView state) noexcept
{
    if (!blitzar_core::IsValid(state)) {
        return false;
    }
    for (std::size_t index = 0; index < state.count; ++index) {
        if (!std::isfinite(state.x[index]) || !std::isfinite(state.y[index]) ||
            !std::isfinite(state.z[index]) || !std::isfinite(state.velocity_x[index]) ||
            !std::isfinite(state.velocity_y[index]) || !std::isfinite(state.velocity_z[index]) ||
            !std::isfinite(state.mass[index]) || state.mass[index] < 0.0) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] inline bool IsFiniteForce(blitzar_core::ForceView force) noexcept
{
    if (!blitzar_core::IsValid(force)) {
        return false;
    }
    for (std::size_t index = 0; index < force.count; ++index) {
        if (!std::isfinite(force.x[index]) || !std::isfinite(force.y[index]) ||
            !std::isfinite(force.z[index])) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] inline blitzar_status RestoreOr(blitzar_integration::LeapfrogWorkspace& workspace,
    blitzar_core::MutableParticleView state, blitzar_status status) noexcept
{
    const blitzar_status restore_status = workspace.Restore(state);

    return restore_status == BLITZAR_STATUS_OK ? status : restore_status;
}

template <typename RollbackHook>
[[nodiscard]] inline blitzar_status RestoreWithRollback(RollbackHook& rollback_hook,
    blitzar_particles::ParticleBuffer& particles, blitzar_integration::LeapfrogWorkspace& workspace,
    blitzar_status status) noexcept
{
    rollback_hook();

    return RestoreOr(workspace, particles.MutableView(), status);
}

template <typename Solver, typename Workspace> struct SolverComputeRequest final {
    Solver& solver;
    blitzar_core::ParticleStateView particles;
    blitzar_core::ForceView force;
    const blitzar_core::ExecutionSettings& settings;
    Workspace& workspace;
};

template <typename Solver, typename Workspace>
[[nodiscard]] inline blitzar_status ComputeSolver(
    const SolverComputeRequest<Solver, Workspace>& request) noexcept
{
    if constexpr (requires(Solver& candidate, blitzar_core::ParticleStateView candidate_particles,
                      blitzar_core::ForceView candidate_force,
                      const blitzar_core::ExecutionSettings& candidate_settings,
                      Workspace& candidate_workspace) {
                      candidate.Compute(candidate_particles, candidate_force, candidate_settings,
                          candidate_workspace);
                  }) {
        return request.solver.Compute(
            request.particles, request.force, request.settings, request.workspace);
    }
    else {
        return request.solver.Compute(request.particles, request.force, request.settings);
    }
}

template <typename Solver, typename SolverWorkspace> struct AdvanceState final {
    blitzar_particles::ParticleBuffer& particles;
    blitzar_particles::AccelerationBuffer& accelerations;
    blitzar_integration::LeapfrogWorkspace& workspace;
    Solver& solver;
    blitzar_core::Scalar timestep{};
    const blitzar_core::ExecutionSettings& settings;
    SolverWorkspace& solver_workspace;
    blitzar_core::ParticleStateView solver_particles;
};

template <typename DriftHook, typename RollbackHook> struct AdvanceHooks final {
    DriftHook& drift;
    RollbackHook& rollback;
};

template <typename Solver, typename SolverWorkspace, typename DriftHook, typename RollbackHook>
struct AdvanceRequest final {
    AdvanceState<Solver, SolverWorkspace>& state;
    AdvanceHooks<DriftHook, RollbackHook>& hooks;
};

} // namespace blitzar_integration_kdk

namespace blitzar_integration {

class LeapfrogKdk final {
public:
    template <typename Solver, typename SolverWorkspace>
    [[nodiscard]] blitzar_status Advance(
        blitzar_integration_kdk::AdvanceState<Solver, SolverWorkspace>& state) const noexcept
    {
        blitzar_integration_kdk::NoopDriftHook drift_hook;
        blitzar_integration_kdk::NoopRollbackHook rollback_hook;
        blitzar_integration_kdk::AdvanceHooks hooks{drift_hook, rollback_hook};
        blitzar_integration_kdk::AdvanceRequest request{state, hooks};

        return Advance(request);
    }

    template <typename Solver, typename SolverWorkspace, typename DriftHook, typename RollbackHook>
    [[nodiscard]] blitzar_status Advance(
        blitzar_integration_kdk::AdvanceRequest<Solver, SolverWorkspace, DriftHook, RollbackHook>&
            request) const noexcept
    {
        auto& state = request.state;

        if (!state.particles.IsValid() || !state.accelerations.IsValid() ||
            !state.workspace.IsValid() ||
            state.particles.Count() != state.accelerations.Count() ||
            state.particles.Count() != state.workspace.Count() || !std::isfinite(state.timestep) ||
            state.timestep <= 0.0 || !state.settings.IsValid() ||
            !blitzar_integration_kdk::IsFiniteState(state.particles.State()) ||
            state.solver_particles.count != state.particles.Count() ||
            !blitzar_integration_kdk::IsFiniteState(state.solver_particles)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        blitzar_core::MutableParticleView mutable_state = state.particles.MutableView();
        blitzar_status status = state.workspace.Capture(mutable_state);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        blitzar_core::ForceView force = state.accelerations.View();

        blitzar_integration_kdk::SolverComputeRequest compute_request{
            state.solver, state.solver_particles, force, state.settings, state.solver_workspace};
        status = blitzar_integration_kdk::ComputeSolver(compute_request);

        if (status != BLITZAR_STATUS_OK) {
            return blitzar_integration_kdk::RestoreWithRollback(
                request.hooks.rollback, state.particles, state.workspace, status);
        }
        if (!blitzar_integration_kdk::IsFiniteForce(force)) {
            return blitzar_integration_kdk::RestoreWithRollback(
                request.hooks.rollback, state.particles, state.workspace,
                BLITZAR_STATUS_INVALID_ARGUMENT);
        }

        const blitzar_core::Scalar half_step = 0.5 * state.timestep;

#if defined(_OPENMP)
#pragma omp parallel for simd schedule(static)
#endif

        for (std::int64_t raw_index = 0;
             raw_index < static_cast<std::int64_t>(state.particles.Count()); ++raw_index) {
            const std::size_t index = static_cast<std::size_t>(raw_index);

            mutable_state.velocity_x[index] += half_step * force.x[index];
            mutable_state.velocity_y[index] += half_step * force.y[index];
            mutable_state.velocity_z[index] += half_step * force.z[index];
            mutable_state.x[index] += state.timestep * mutable_state.velocity_x[index];
            mutable_state.y[index] += state.timestep * mutable_state.velocity_y[index];
            mutable_state.z[index] += state.timestep * mutable_state.velocity_z[index];
        }
        if (!blitzar_integration_kdk::IsFiniteState(state.particles.State())) {
            return blitzar_integration_kdk::RestoreWithRollback(
                request.hooks.rollback, state.particles, state.workspace,
                BLITZAR_STATUS_INVALID_ARGUMENT);
        }

        const blitzar_integration_kdk::DriftTransition transition =
            request.hooks.drift(state.particles, state.accelerations, state.workspace);

        if (transition.status != BLITZAR_STATUS_OK) {
            return blitzar_integration_kdk::RestoreWithRollback(
                request.hooks.rollback, state.particles, state.workspace, transition.status);
        }
        if (transition.state_replaced) {
            const std::size_t checkpoint_count = state.workspace.Count();

            if (state.workspace.SetCount(state.particles.Count()) != BLITZAR_STATUS_OK) {
                return blitzar_integration_kdk::RestoreWithRollback(
                    request.hooks.rollback, state.particles, state.workspace,
                    BLITZAR_STATUS_INTERNAL_ERROR);
            }

            mutable_state = state.particles.MutableView();

            if (state.workspace.Capture(mutable_state) != BLITZAR_STATUS_OK) {
                (void)state.workspace.SetCount(checkpoint_count);

                return blitzar_integration_kdk::RestoreWithRollback(
                    request.hooks.rollback, state.particles, state.workspace,
                    BLITZAR_STATUS_INTERNAL_ERROR);
            }

            state.solver_particles = state.particles.State();
        }

        mutable_state = state.particles.MutableView();
        force = state.accelerations.View();
        const blitzar_integration_kdk::SolverComputeRequest second_compute_request{
            state.solver, state.solver_particles, force, state.settings, state.solver_workspace};
        status = blitzar_integration_kdk::ComputeSolver(second_compute_request);

        if (status != BLITZAR_STATUS_OK) {
            return blitzar_integration_kdk::RestoreWithRollback(
                request.hooks.rollback, state.particles, state.workspace, status);
        }
        if (!blitzar_integration_kdk::IsFiniteForce(force)) {
            return blitzar_integration_kdk::RestoreWithRollback(
                request.hooks.rollback, state.particles, state.workspace,
                BLITZAR_STATUS_INVALID_ARGUMENT);
        }
#if defined(_OPENMP)
#pragma omp parallel for simd schedule(static)
#endif
        for (std::int64_t raw_index = 0;
             raw_index < static_cast<std::int64_t>(state.particles.Count()); ++raw_index) {
            const std::size_t index = static_cast<std::size_t>(raw_index);

            mutable_state.velocity_x[index] += half_step * force.x[index];
            mutable_state.velocity_y[index] += half_step * force.y[index];
            mutable_state.velocity_z[index] += half_step * force.z[index];
        }
        if (!blitzar_integration_kdk::IsFiniteState(state.particles.State())) {
            return blitzar_integration_kdk::RestoreWithRollback(
                request.hooks.rollback, state.particles, state.workspace,
                BLITZAR_STATUS_INVALID_ARGUMENT);
        }

        return BLITZAR_STATUS_OK;
    }
};

} // namespace blitzar_integration

#endif
