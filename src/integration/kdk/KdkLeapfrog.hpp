#ifndef BLITZAR_INTEGRATION_KDK_KDK_LEAPFROG_HPP
#define BLITZAR_INTEGRATION_KDK_KDK_LEAPFROG_HPP

#include "core/CoreExecution.hpp"
#include "integration/kdk/KdkCheckpoint.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "solvers/SolverForceEvaluation.hpp"

#include <cmath>
#include <cstdint>

namespace blitzar_integration_kdk {

struct DriftTransition final {
    blitzar_status status{BLITZAR_STATUS_OK};
    bool state_replaced{false};
};

struct NoopDriftHook final {
    [[nodiscard]] DriftTransition operator()(blitzar_particles::ParticleBuffer&,
        blitzar_particles::ParticleAccelerationBuffer&,
        blitzar_integration::KdkCheckpoint&) const noexcept
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

template <typename ForceProvider> struct AdvanceState final {
    blitzar_particles::ParticleBuffer& particles;
    blitzar_particles::ParticleAccelerationBuffer& accelerations;
    blitzar_integration::KdkCheckpoint& checkpoint;
    ForceProvider& force_provider;
    blitzar_core::Scalar timestep{};
    const blitzar_core::ExecutionSettings& settings;
    blitzar_core::ParticleStateView solver_particles;
};

template <typename DriftHook, typename RollbackHook> struct AdvanceHooks final {
    DriftHook& drift;
    RollbackHook& rollback;
};

template <typename ForceProvider, typename DriftHook, typename RollbackHook>
struct AdvanceRequest final {
    AdvanceState<ForceProvider>& state;
    AdvanceHooks<DriftHook, RollbackHook>& hooks;
};

} // namespace blitzar_integration_kdk

namespace blitzar_integration {

class KdkLeapfrog final {
public:
    template <typename ForceProvider>
    [[nodiscard]] blitzar_status Advance(
        blitzar_integration_kdk::AdvanceState<ForceProvider>& state) const noexcept;

    template <typename ForceProvider, typename DriftHook, typename RollbackHook>
    [[nodiscard]] blitzar_status Advance(
        blitzar_integration_kdk::AdvanceRequest<ForceProvider, DriftHook, RollbackHook>& request)
        const noexcept;
};

} // namespace blitzar_integration

#include "integration/kdk/KdkAdvance.hpp"

#endif
