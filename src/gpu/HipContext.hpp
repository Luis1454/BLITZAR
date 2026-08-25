#ifndef BLITZAR_GPU_HIP_CONTEXT_HPP
#define BLITZAR_GPU_HIP_CONTEXT_HPP

#include "core/Execution.hpp"
#include "core/Solver.hpp"
#include "core/Types.hpp"
#include "physics/GravityLaw.hpp"
#include "solvers/barnes_hut/BarnesHutSolver.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace blitzar_gpu {

enum class HipFault : std::uint8_t {
    None,
    AllocationFailure,
    LaunchFailure,
    SynchronizationFailure,
    NonFiniteResult,
};

struct BarnesHutComputeRequest final {
    blitzar_core::ParticleStateView particles;
    blitzar_core::ForceView forces;
    const blitzar_core::ExecutionSettings& execution;
    blitzar_physics::GravityParameters gravity;
    blitzar_barnes_hut::BarnesHutSettings settings;
};

class HipContext final {
public:
    HipContext() noexcept;
    ~HipContext() noexcept;

    HipContext(const HipContext&) = delete;
    HipContext& operator=(const HipContext&) = delete;

    HipContext(HipContext&& other) noexcept;
    HipContext& operator=(HipContext&& other) noexcept;

    [[nodiscard]] bool IsCompiled() const noexcept;
    [[nodiscard]] bool IsAvailable() const noexcept;
    void SetFaultForTesting(HipFault fault) noexcept;

    [[nodiscard]] blitzar_status ComputeDirect(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, blitzar_physics::GravityParameters gravity) noexcept;
    [[nodiscard]] blitzar_status ComputeDirectRange(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, blitzar_physics::GravityParameters gravity,
        blitzar_core::ForceRange range) noexcept;

    [[nodiscard]] blitzar_status ComputeBarnesHut(
        const BarnesHutComputeRequest& request) noexcept;

private:
    struct Impl;

    [[nodiscard]] blitzar_status CheckRuntime() const noexcept;

    std::unique_ptr<Impl> impl_;
    blitzar_status status_{BLITZAR_STATUS_OK};
};

} // namespace blitzar_gpu

#endif
