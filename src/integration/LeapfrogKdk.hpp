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

template <typename Solver, typename Workspace>
[[nodiscard]] inline blitzar_status ComputeSolver(Solver& solver,
    blitzar_core::ParticleStateView particles, blitzar_core::ForceView force,
    const blitzar_core::ExecutionSettings& settings, Workspace& workspace) noexcept
{
    if constexpr (requires(Solver& candidate, blitzar_core::ParticleStateView candidate_particles,
                      blitzar_core::ForceView candidate_force,
                      const blitzar_core::ExecutionSettings& candidate_settings,
                      Workspace& candidate_workspace) {
                      candidate.Compute(candidate_particles, candidate_force, candidate_settings,
                          candidate_workspace);
                  }) {
        return solver.Compute(particles, force, settings, workspace);
    }
    else {
        return solver.Compute(particles, force, settings);
    }
}

} // namespace blitzar_integration_kdk

namespace blitzar_integration {

class LeapfrogKdk final {
public:
    template <typename Solver>
    [[nodiscard]] blitzar_status Advance(blitzar_particles::ParticleBuffer& particles,
        blitzar_particles::AccelerationBuffer& accelerations, LeapfrogWorkspace& workspace,
        Solver& solver, blitzar_core::Scalar timestep,
        const blitzar_core::ExecutionSettings& settings) const noexcept
    {
        std::span<std::size_t> solver_workspace{};
        return Advance(particles, accelerations, workspace, solver, timestep, settings,
            solver_workspace, particles.State());
    }

    template <typename Solver, typename SolverWorkspace>
    [[nodiscard]] blitzar_status Advance(blitzar_particles::ParticleBuffer& particles,
        blitzar_particles::AccelerationBuffer& accelerations, LeapfrogWorkspace& workspace,
        Solver& solver, blitzar_core::Scalar timestep,
        const blitzar_core::ExecutionSettings& settings,
        SolverWorkspace& solver_workspace) const noexcept
    {
        return Advance(particles, accelerations, workspace, solver, timestep, settings,
            solver_workspace, particles.State());
    }

    template <typename Solver, typename SolverWorkspace>
    [[nodiscard]] blitzar_status Advance(blitzar_particles::ParticleBuffer& particles,
        blitzar_particles::AccelerationBuffer& accelerations, LeapfrogWorkspace& workspace,
        Solver& solver, blitzar_core::Scalar timestep,
        const blitzar_core::ExecutionSettings& settings, SolverWorkspace& solver_workspace,
        blitzar_core::ParticleStateView solver_particles) const noexcept
    {
        blitzar_integration_kdk::NoopDriftHook drift_hook;
        return Advance(particles, accelerations, workspace, solver, timestep, settings,
            solver_workspace, solver_particles, drift_hook);
    }

    template <typename Solver, typename SolverWorkspace, typename DriftHook>
    [[nodiscard]] blitzar_status Advance(blitzar_particles::ParticleBuffer& particles,
        blitzar_particles::AccelerationBuffer& accelerations, LeapfrogWorkspace& workspace,
        Solver& solver, blitzar_core::Scalar timestep,
        const blitzar_core::ExecutionSettings& settings, SolverWorkspace& solver_workspace,
        blitzar_core::ParticleStateView solver_particles, DriftHook& drift_hook) const noexcept
    {
        blitzar_integration_kdk::NoopRollbackHook rollback_hook;
        return Advance(particles, accelerations, workspace, solver, timestep, settings,
            solver_workspace, solver_particles, drift_hook, rollback_hook);
    }

    template <typename Solver, typename SolverWorkspace, typename DriftHook, typename RollbackHook>

    [[nodiscard]] blitzar_status Advance(blitzar_particles::ParticleBuffer& particles,
        blitzar_particles::AccelerationBuffer& accelerations, LeapfrogWorkspace& workspace,
        Solver& solver, blitzar_core::Scalar timestep,
        const blitzar_core::ExecutionSettings& settings, SolverWorkspace& solver_workspace,
        blitzar_core::ParticleStateView solver_particles, DriftHook& drift_hook,
        RollbackHook& rollback_hook) const noexcept
    {
        if (!particles.IsValid() || !accelerations.IsValid() || !workspace.IsValid() ||
            particles.Count() != accelerations.Count() || particles.Count() != workspace.Count() ||
            !std::isfinite(timestep) || timestep <= 0.0 || !settings.IsValid() ||
            !blitzar_integration_kdk::IsFiniteState(particles.State()) ||
            solver_particles.count != particles.Count() ||
            !blitzar_integration_kdk::IsFiniteState(solver_particles)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        blitzar_core::MutableParticleView mutable_state = particles.MutableView();
        blitzar_status status = workspace.Capture(mutable_state);
        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
        blitzar_core::ForceView force = accelerations.View();
        status = blitzar_integration_kdk::ComputeSolver(
            solver, solver_particles, force, settings, solver_workspace);
        if (status != BLITZAR_STATUS_OK) {
            return blitzar_integration_kdk::RestoreWithRollback(
                rollback_hook, particles, workspace, status);
        }
        if (!blitzar_integration_kdk::IsFiniteForce(force)) {
            return blitzar_integration_kdk::RestoreWithRollback(
                rollback_hook, particles, workspace, BLITZAR_STATUS_INVALID_ARGUMENT);
        }

        const blitzar_core::Scalar half_step = 0.5 * timestep;

#if defined(_OPENMP)
#pragma omp parallel for simd schedule(static)
#endif
        for (std::int64_t raw_index = 0; raw_index < static_cast<std::int64_t>(particles.Count());
             ++raw_index) {
            const std::size_t index = static_cast<std::size_t>(raw_index);

            mutable_state.velocity_x[index] += half_step * force.x[index];
            mutable_state.velocity_y[index] += half_step * force.y[index];
            mutable_state.velocity_z[index] += half_step * force.z[index];
            mutable_state.x[index] += timestep * mutable_state.velocity_x[index];
            mutable_state.y[index] += timestep * mutable_state.velocity_y[index];
            mutable_state.z[index] += timestep * mutable_state.velocity_z[index];
        }
        if (!blitzar_integration_kdk::IsFiniteState(particles.State())) {
            return blitzar_integration_kdk::RestoreWithRollback(
                rollback_hook, particles, workspace, BLITZAR_STATUS_INVALID_ARGUMENT);
        }

        const blitzar_integration_kdk::DriftTransition transition =
            drift_hook(particles, accelerations, workspace);
        if (transition.status != BLITZAR_STATUS_OK) {
            return blitzar_integration_kdk::RestoreWithRollback(
                rollback_hook, particles, workspace, transition.status);
        }
        if (transition.state_replaced) {
            const std::size_t checkpoint_count = workspace.Count();

            if (workspace.SetCount(particles.Count()) != BLITZAR_STATUS_OK) {
                return blitzar_integration_kdk::RestoreWithRollback(
                    rollback_hook, particles, workspace, BLITZAR_STATUS_INTERNAL_ERROR);
            }

            mutable_state = particles.MutableView();

            if (workspace.Capture(mutable_state) != BLITZAR_STATUS_OK) {
                (void)workspace.SetCount(checkpoint_count);

                return blitzar_integration_kdk::RestoreWithRollback(
                    rollback_hook, particles, workspace, BLITZAR_STATUS_INTERNAL_ERROR);
            }

            solver_particles = particles.State();
        }
        mutable_state = particles.MutableView();
        force = accelerations.View();

        status = blitzar_integration_kdk::ComputeSolver(
            solver, solver_particles, force, settings, solver_workspace);
        if (status != BLITZAR_STATUS_OK) {
            return blitzar_integration_kdk::RestoreWithRollback(
                rollback_hook, particles, workspace, status);
        }
        if (!blitzar_integration_kdk::IsFiniteForce(force)) {
            return blitzar_integration_kdk::RestoreWithRollback(
                rollback_hook, particles, workspace, BLITZAR_STATUS_INVALID_ARGUMENT);
        }
#if defined(_OPENMP)
#pragma omp parallel for simd schedule(static)
#endif
        for (std::int64_t raw_index = 0; raw_index < static_cast<std::int64_t>(particles.Count());
             ++raw_index) {
            const std::size_t index = static_cast<std::size_t>(raw_index);

            mutable_state.velocity_x[index] += half_step * force.x[index];
            mutable_state.velocity_y[index] += half_step * force.y[index];
            mutable_state.velocity_z[index] += half_step * force.z[index];
        }
        if (!blitzar_integration_kdk::IsFiniteState(particles.State())) {
            return blitzar_integration_kdk::RestoreWithRollback(
                rollback_hook, particles, workspace, BLITZAR_STATUS_INVALID_ARGUMENT);
        }
        return BLITZAR_STATUS_OK;
    }
};

} // namespace blitzar_integration

#endif
