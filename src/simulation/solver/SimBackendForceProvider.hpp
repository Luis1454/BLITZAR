#ifndef BLITZAR_SIMULATION_SOLVER_SIM_BACKEND_FORCE_PROVIDER_HPP
#define BLITZAR_SIMULATION_SOLVER_SIM_BACKEND_FORCE_PROVIDER_HPP

#include "gpu/runtime/GpuContext.hpp"
#include "solvers/SolverCpuForceProvider.hpp"

#include <atomic>
#include <blitzar/blitzar.h>

namespace blitzar_sim {

template <typename Solver> struct SimBackendForceContext final {
    blitzar_hip::GpuContext& accelerator;
    Solver& cpu;
    blitzar_physics::GravityParameters gravity;
    blitzar_barnes_hut::BarnesHutSettings barnes_hut;
    std::atomic<blitzar_backend_kind>& backend;
};

template <typename Solver> struct SimBackendForceTraits {
    [[nodiscard]] static blitzar_status TryGpu(const SimBackendForceContext<Solver>&,
        const blitzar_solvers::SolverForceEvaluation&) noexcept
    {
        return BLITZAR_STATUS_UNSUPPORTED;
    }
};

template <> struct SimBackendForceTraits<blitzar_direct::DirectSolver> final {
    [[nodiscard]] static blitzar_status TryGpu(
        const SimBackendForceContext<blitzar_direct::DirectSolver>& context,
        const blitzar_solvers::SolverForceEvaluation& request) noexcept
    {
        if (request.source_kind != blitzar_solvers::SolverForceSourceKind::Local) {
            return BLITZAR_STATUS_UNSUPPORTED;
        }

        context.backend.store(BLITZAR_BACKEND_HIP, std::memory_order_relaxed);

        return context.accelerator.ComputeDirect(request.targets, request.forces, context.gravity);
    }
};

template <> struct SimBackendForceTraits<blitzar_barnes_hut::BhSolver> final {
    [[nodiscard]] static blitzar_status TryGpu(
        const SimBackendForceContext<blitzar_barnes_hut::BhSolver>& context,
        const blitzar_solvers::SolverForceEvaluation& request) noexcept
    {
        if (request.source_kind != blitzar_solvers::SolverForceSourceKind::Local) {
            return BLITZAR_STATUS_UNSUPPORTED;
        }

        context.backend.store(BLITZAR_BACKEND_HIP, std::memory_order_relaxed);

        return context.accelerator.ComputeBarnesHut({request.targets, request.forces,
            request.settings, context.gravity, context.barnes_hut});
    }
};

template <> struct SimBackendForceTraits<blitzar_fmm::FmmSolver> final {
    [[nodiscard]] static blitzar_status TryGpu(
        const SimBackendForceContext<blitzar_fmm::FmmSolver>&,
        const blitzar_solvers::SolverForceEvaluation&) noexcept
    {
        return BLITZAR_STATUS_UNSUPPORTED;
    }
};

template <> struct SimBackendForceTraits<blitzar_kifmm::KifmmSolver> final {
    [[nodiscard]] static blitzar_status TryGpu(
        const SimBackendForceContext<blitzar_kifmm::KifmmSolver>&,
        const blitzar_solvers::SolverForceEvaluation&) noexcept
    {
        return BLITZAR_STATUS_UNSUPPORTED;
    }
};

template <typename Solver> class SimBackendForceProvider final {
public:
    explicit SimBackendForceProvider(SimBackendForceContext<Solver> context) noexcept
        : context_(context)
    {
    }

    [[nodiscard]] blitzar_status Evaluate(
        const blitzar_solvers::SolverForceEvaluation& request) noexcept
    {
        const blitzar_status gpu_status = SimBackendForceTraits<Solver>::TryGpu(context_, request);

        if (gpu_status == BLITZAR_STATUS_OK) {
            return gpu_status;
        }
        if (gpu_status != BLITZAR_STATUS_UNSUPPORTED) {
            return gpu_status;
        }

        context_.backend.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

        blitzar_solvers::SolverCpuForceProvider<Solver> cpu(context_.cpu);

        return cpu.Evaluate(request);
    }

private:
    SimBackendForceContext<Solver> context_;
};

} // namespace blitzar_sim

#endif
