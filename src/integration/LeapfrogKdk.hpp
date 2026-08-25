#ifndef BLITZAR_INTEGRATION_LEAPFROG_KDK_HPP
#define BLITZAR_INTEGRATION_LEAPFROG_KDK_HPP

#include "core/Execution.hpp"
#include "integration/KdkCheckpoint.hpp"
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
        blitzar_particles::AccelerationBuffer&, blitzar_integration::KdkCheckpoint&) const noexcept
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

[[nodiscard]] inline blitzar_status RestoreOr(blitzar_integration::KdkCheckpoint& checkpoint,
    blitzar_core::MutableParticleView state, blitzar_status status) noexcept
{
    const blitzar_status restore_status = checkpoint.Restore(state);

    return restore_status == BLITZAR_STATUS_OK ? status : restore_status;
}

template <typename RollbackHook>
[[nodiscard]] inline blitzar_status RestoreWithRollback(RollbackHook& rollback_hook,
    blitzar_particles::ParticleBuffer& particles, blitzar_integration::KdkCheckpoint& checkpoint,
    blitzar_status status) noexcept
{
    rollback_hook();

    return RestoreOr(checkpoint, particles.MutableView(), status);
}

template <typename Solver, typename Scratch> struct SolverComputeRequest final {
    Solver& solver;
    blitzar_core::ParticleStateView particles;
    blitzar_core::ForceView force;
    const blitzar_core::ExecutionSettings& settings;
    Scratch& scratch;
};

template <typename Solver, typename Scratch>
[[nodiscard]] inline blitzar_status ComputeSolver(
    const SolverComputeRequest<Solver, Scratch>& request) noexcept
{
    if constexpr (requires(Solver& candidate, blitzar_core::ParticleStateView candidate_particles,
                      blitzar_core::ForceView candidate_force,
                      const blitzar_core::ExecutionSettings& candidate_settings,
                      Scratch& candidate_scratch) {
                      candidate.Compute(candidate_particles, candidate_force, candidate_settings,
                          candidate_scratch);
                  }) {
        return request.solver.Compute(
            request.particles, request.force, request.settings, request.scratch);
    }
    else {
        return request.solver.Compute(request.particles, request.force, request.settings);
    }
}

template <typename Solver, typename SolverScratch> struct AdvanceState final {
    blitzar_particles::ParticleBuffer& particles;
    blitzar_particles::AccelerationBuffer& accelerations;
    blitzar_integration::KdkCheckpoint& checkpoint;
    Solver& solver;
    blitzar_core::Scalar timestep{};
    const blitzar_core::ExecutionSettings& settings;
    SolverScratch& solver_scratch;
    blitzar_core::ParticleStateView solver_particles;
};

template <typename DriftHook, typename RollbackHook> struct AdvanceHooks final {
    DriftHook& drift;
    RollbackHook& rollback;
};

template <typename Solver, typename SolverScratch, typename DriftHook, typename RollbackHook>
struct AdvanceRequest final {
    AdvanceState<Solver, SolverScratch>& state;
    AdvanceHooks<DriftHook, RollbackHook>& hooks;
};

} // namespace blitzar_integration_kdk

namespace blitzar_integration {

class LeapfrogKdk final {
public:
    template <typename Solver, typename SolverScratch>
    [[nodiscard]] blitzar_status Advance(
        blitzar_integration_kdk::AdvanceState<Solver, SolverScratch>& state) const noexcept;

    template <typename Solver, typename SolverScratch, typename DriftHook, typename RollbackHook>
    [[nodiscard]] blitzar_status Advance(
        blitzar_integration_kdk::AdvanceRequest<Solver, SolverScratch, DriftHook, RollbackHook>&
            request) const noexcept;
};

} // namespace blitzar_integration

#include "integration/KdkAdvance.hpp"

#endif
