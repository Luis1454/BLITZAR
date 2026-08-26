#ifndef BLITZAR_ACCELERATORS_GPU_HIP_RUNTIME_CONTEXT_HPP
#define BLITZAR_ACCELERATORS_GPU_HIP_RUNTIME_CONTEXT_HPP

#include "core/contracts/Execution.hpp"
#include "core/contracts/Types.hpp"
#include "physics/gravity/GravityLaw.hpp"
#include "solvers/barnes_hut/BarnesHutSolver.hpp"
#include "solvers/contracts/SolverContract.hpp"

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

class Context final {
public:
    Context() noexcept;
    ~Context() noexcept;

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    Context(Context&& other) noexcept;
    Context& operator=(Context&& other) noexcept;

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
