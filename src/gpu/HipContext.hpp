#ifndef BLITZAR_GPU_HIP_CONTEXT_HPP
#define BLITZAR_GPU_HIP_CONTEXT_HPP

#include "core/Execution.hpp"
#include "core/Types.hpp"
#include "physics/GravityLaw.hpp"
#include "solvers/barnes_hut/BarnesHutSolver.hpp"

#include <blitzar/blitzar.h>

#include <cstddef>
#include <memory>

namespace blitzar_gpu {

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

    [[nodiscard]] blitzar_status ComputeDirect(
        blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces,
        blitzar_physics::GravityParameters gravity) noexcept;

    [[nodiscard]] blitzar_status ComputeBarnesHut(
        blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& execution,
        blitzar_physics::GravityParameters gravity,
        blitzar_barnes_hut::BarnesHutSettings settings) noexcept;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

}  // namespace blitzar_gpu

#endif
