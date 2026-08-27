#ifndef BLITZAR_GPU_RUNTIME_GPU_CONTEXT_HPP
#define BLITZAR_GPU_RUNTIME_GPU_CONTEXT_HPP

#include "core/CoreExecution.hpp"
#include "core/CoreTypes.hpp"
#include "physics/gravity/GravityLaw.hpp"
#include "solvers/SolverContract.hpp"
#include "solvers/barnes_hut/BhSolver.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace blitzar_hip {

enum class Fault : std::uint8_t {
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

class GpuContext final {
public:
    GpuContext() noexcept;
    ~GpuContext() noexcept;

    GpuContext(const GpuContext&) = delete;
    GpuContext& operator=(const GpuContext&) = delete;

    GpuContext(GpuContext&& other) noexcept;
    GpuContext& operator=(GpuContext&& other) noexcept;

    [[nodiscard]] bool IsCompiled() const noexcept;
    [[nodiscard]] bool IsAvailable() const noexcept;
    void SetFaultForTesting(Fault fault) noexcept;

    [[nodiscard]] blitzar_status ComputeDirect(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, blitzar_physics::GravityParameters gravity) noexcept;
    [[nodiscard]] blitzar_status ComputeDirectRange(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, blitzar_physics::GravityParameters gravity,
        blitzar_solvers::ForceRange range) noexcept;

    [[nodiscard]] blitzar_status ComputeBarnesHut(const BarnesHutComputeRequest& request) noexcept;

private:
    struct Impl;

    [[nodiscard]] blitzar_status CheckRuntime() const noexcept;

    std::unique_ptr<Impl> impl_;
    blitzar_status status_{BLITZAR_STATUS_OK};
};

} // namespace blitzar_hip

#endif
