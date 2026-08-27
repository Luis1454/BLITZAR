#ifndef BLITZAR_SIMULATION_SOLVER_SIM_SOLVER_DISPATCH_HPP
#define BLITZAR_SIMULATION_SOLVER_SIM_SOLVER_DISPATCH_HPP

#include "core/CoreExecution.hpp"
#include "gpu/runtime/GpuContext.hpp"
#include "solvers/barnes_hut/BhSolver.hpp"
#include "solvers/direct/DirectSolver.hpp"
#include "solvers/fmm/FmmSolver.hpp"
#include "solvers/threading/ThreadStackPool.hpp"

#include <atomic>
#include <blitzar/blitzar.h>
#include <type_traits>

namespace blitzar_sim {

template <typename Solver> struct SolverDispatchContext final {
    blitzar_hip::GpuContext& accelerator;
    Solver& cpu;
    blitzar_physics::GravityParameters gravity;
    blitzar_barnes_hut::BarnesHutSettings barnes_hut;
    std::atomic<blitzar_backend_kind>& backend;
};

template <typename Solver> class SolverDispatcher final {
public:
    explicit SolverDispatcher(SolverDispatchContext<Solver> context) noexcept
        : accelerator_(context.accelerator), cpu_(context.cpu), gravity_(context.gravity),
          barnes_hut_(context.barnes_hut), backend_(context.backend)
    {
    }

    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            backend_.store(BLITZAR_BACKEND_HIP, std::memory_order_relaxed);

            const blitzar_status gpu_status =
                accelerator_.ComputeDirect(particles, forces, gravity_);

            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
            if (gpu_status != BLITZAR_STATUS_UNSUPPORTED) {
                return gpu_status;
            }
        }
        else if constexpr (std::is_same_v<Solver, blitzar_barnes_hut::BhSolver>) {
            backend_.store(BLITZAR_BACKEND_HIP, std::memory_order_relaxed);

            const blitzar_status gpu_status =
                accelerator_.ComputeBarnesHut({particles, forces, settings, gravity_, barnes_hut_});

            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
            if (gpu_status != BLITZAR_STATUS_UNSUPPORTED) {
                return gpu_status;
            }
        }
        else {
            backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

            return cpu_.Compute(particles, forces, settings);
        }

        backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

        return cpu_.Compute(particles, forces, settings);
    }

    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        blitzar_solver_threading::ThreadStackPool& stack_pool) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            backend_.store(BLITZAR_BACKEND_HIP, std::memory_order_relaxed);

            const blitzar_status gpu_status =
                accelerator_.ComputeDirect(particles, forces, gravity_);

            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
            if (gpu_status != BLITZAR_STATUS_UNSUPPORTED) {
                return gpu_status;
            }

            backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

            return cpu_.Compute(particles, forces, settings);
        }
        else if constexpr (std::is_same_v<Solver, blitzar_barnes_hut::BhSolver>) {
            backend_.store(BLITZAR_BACKEND_HIP, std::memory_order_relaxed);

            const blitzar_status gpu_status =
                accelerator_.ComputeBarnesHut({particles, forces, settings, gravity_, barnes_hut_});

            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
            if (gpu_status != BLITZAR_STATUS_UNSUPPORTED) {
                return gpu_status;
            }

            backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

            return cpu_.Compute(particles, forces, settings, stack_pool);
        }
        else {
            backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

            return cpu_.Compute(particles, forces, settings);
        }
    }

    [[nodiscard]] blitzar_status ComputeRange(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        blitzar_solvers::ForceRange range) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            backend_.store(BLITZAR_BACKEND_HIP, std::memory_order_relaxed);

            const blitzar_status gpu_status =
                accelerator_.ComputeDirectRange(particles, forces, gravity_, range);

            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
            if (gpu_status != BLITZAR_STATUS_UNSUPPORTED) {
                return gpu_status;
            }

            backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

            return cpu_.ComputeRange(particles, forces, settings, range);
        }
        else {
            return BLITZAR_STATUS_UNSUPPORTED;
        }
    }

    [[nodiscard]] blitzar_status ComputeRemote(blitzar_core::ParticleStateView targets,
        blitzar_core::ParticleStateView sources, blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

            return cpu_.ComputeRemote(targets, sources, forces, settings);
        }
        else {
            return BLITZAR_STATUS_UNSUPPORTED;
        }
    }

    [[nodiscard]] blitzar_status ComputeSplit(
        const blitzar_barnes_hut::BarnesHutSplitRequest& request) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_barnes_hut::BhSolver> ||
                      std::is_same_v<Solver, blitzar_fmm::FmmSolver>) {
            backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

            return cpu_.ComputeSplit(request);
        }
        else {
            return BLITZAR_STATUS_UNSUPPORTED;
        }
    }

    [[nodiscard]] blitzar_status ComputeSplit(
        const blitzar_barnes_hut::BarnesHutSplitRequest& request,
        blitzar_solver_threading::ThreadStackPool& stack_pool) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_barnes_hut::BhSolver> ||
                      std::is_same_v<Solver, blitzar_fmm::FmmSolver>) {
            backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

            return cpu_.ComputeSplit(request, stack_pool);
        }
        else {
            return BLITZAR_STATUS_UNSUPPORTED;
        }
    }

private:
    blitzar_hip::GpuContext& accelerator_;
    Solver& cpu_;
    blitzar_physics::GravityParameters gravity_;
    blitzar_barnes_hut::BarnesHutSettings barnes_hut_;
    std::atomic<blitzar_backend_kind>& backend_;
};

} // namespace blitzar_sim

#endif
