#ifndef BLITZAR_SDK_SRV_DISPATCH_HPP
#define BLITZAR_SDK_SRV_DISPATCH_HPP

#include "core/Execution.hpp"
#include "core/Solver.hpp"
#include "gpu/HipContext.hpp"
#include "parallel/MpiExchange.hpp"
#include "sdk/SrvState.hpp"
#include "solvers/barnes_hut/BarnesHutSolver.hpp"
#include "solvers/barnes_hut/ThreadWorkspace.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <atomic>
#include <blitzar/blitzar.h>
#include <cstdint>
#include <span>
#include <type_traits>

namespace blitzar_sdk {

template <typename Solver> struct SrvSolverDispatchContext final {
    blitzar_gpu::HipContext& hip;
    Solver& cpu;
    blitzar_physics::GravityParameters gravity;
    blitzar_barnes_hut::BarnesHutSettings barnes_hut;
    std::atomic<blitzar_backend_kind>& backend;
};

template <typename Solver> class SrvSolverDispatcher final {
public:
    explicit SrvSolverDispatcher(SrvSolverDispatchContext<Solver> context) noexcept
        : hip_(context.hip), cpu_(context.cpu), gravity_(context.gravity),
          barnes_hut_(context.barnes_hut), backend_(context.backend)
    {
    }

    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            backend_.store(BLITZAR_BACKEND_HIP, std::memory_order_relaxed);

            const blitzar_status gpu_status = hip_.ComputeDirect(particles, forces, gravity_);

            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
            if (gpu_status != BLITZAR_STATUS_UNSUPPORTED) {
                return gpu_status;
            }
        }
        else {
            backend_.store(BLITZAR_BACKEND_HIP, std::memory_order_relaxed);

            const blitzar_status gpu_status =
                hip_.ComputeBarnesHut({particles, forces, settings, gravity_, barnes_hut_});

            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
            if (gpu_status != BLITZAR_STATUS_UNSUPPORTED) {
                return gpu_status;
            }
        }

        backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

        return cpu_.Compute(particles, forces, settings);
    }

    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        blitzar_barnes_hut::ThreadWorkspace& workspace) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            backend_.store(BLITZAR_BACKEND_HIP, std::memory_order_relaxed);

            const blitzar_status gpu_status = hip_.ComputeDirect(particles, forces, gravity_);

            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
            if (gpu_status != BLITZAR_STATUS_UNSUPPORTED) {
                return gpu_status;
            }

            backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

            return cpu_.Compute(particles, forces, settings);
        }
        else {
            backend_.store(BLITZAR_BACKEND_HIP, std::memory_order_relaxed);

            const blitzar_status gpu_status =
                hip_.ComputeBarnesHut({particles, forces, settings, gravity_, barnes_hut_});

            if (gpu_status == BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_OK;
            }
            if (gpu_status != BLITZAR_STATUS_UNSUPPORTED) {
                return gpu_status;
            }

            backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

            return cpu_.Compute(particles, forces, settings, workspace);
        }
    }

    [[nodiscard]] blitzar_status ComputeRange(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        blitzar_core::ForceRange range) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            backend_.store(BLITZAR_BACKEND_HIP, std::memory_order_relaxed);

            const blitzar_status gpu_status =
                hip_.ComputeDirectRange(particles, forces, gravity_, range);

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

private:
    blitzar_gpu::HipContext& hip_;
    Solver& cpu_;
    blitzar_physics::GravityParameters gravity_;
    blitzar_barnes_hut::BarnesHutSettings barnes_hut_;
    std::atomic<blitzar_backend_kind>& backend_;
};

template <typename Solver> class SrvDistributedDispatcher final {
public:
    struct State final {
        SrvSolverDispatchContext<Solver> solver;
        blitzar_parallel::MpiExchange& exchange;
        blitzar_particles::ParticleArena& arena;
        std::span<const std::uint64_t> ids;
        blitzar_parallel::PacketBuffer& ghosts;
        blitzar_parallel::MpiContext::GhostExchange& halo;
        std::size_t& source_count;
    };

    explicit SrvDistributedDispatcher(State state) noexcept
        : base_(state.solver), exchange_(state.exchange), arena_(state.arena), ids_(state.ids),
          ghosts_(state.ghosts), halo_(state.halo), source_count_(state.source_count)
    {
    }

    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView local_state,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept
    {
        return ComputeWithOverlap(local_state, forces, settings, {});
    }

    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView local_state,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        blitzar_barnes_hut::ThreadWorkspace& workspace) noexcept
    {
        return ComputeWithOverlap(local_state, forces, settings,
            std::span<blitzar_barnes_hut::ThreadWorkspace>(&workspace, 1));
    }

    void Abort() noexcept
    {
        exchange_.AbortGhosts(halo_, ghosts_);
    }

private:
    [[nodiscard]] blitzar_status ComputeWithOverlap(blitzar_core::ParticleStateView local_state,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        std::span<blitzar_barnes_hut::ThreadWorkspace> workspace) noexcept
    {
        const bool ids_valid = ids_.size() >= local_state.count;
        const std::span<const std::uint64_t> local_ids =
            ids_valid ? ids_.first(local_state.count) : std::span<const std::uint64_t>{};
        const blitzar_status begin_status = exchange_.BeginGhosts(local_state, local_ids, halo_);

        if (begin_status != BLITZAR_STATUS_OK) {
            return begin_status;
        }

        blitzar_status local_status = BLITZAR_STATUS_OK;

        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            local_status = base_.ComputeRange(
                local_state, forces, settings, {0, local_state.count, false});
        }
        else if (!workspace.empty()) {
            local_status = base_.Compute(local_state, forces, settings, workspace.front());
        }
        else {
            local_status = base_.Compute(local_state, forces, settings);
        }

        const blitzar_status complete_status = exchange_.CompleteGhosts(halo_, ghosts_);

        if (complete_status != BLITZAR_STATUS_OK) {
            return complete_status;
        }

        const blitzar_status synchronized_local_status =
            exchange_.SynchronizeStatus(local_status, "force-local");

        if (synchronized_local_status != BLITZAR_STATUS_OK) {
            return synchronized_local_status;
        }

        const blitzar_status append_status =
            SrvAppendGhosts(ghosts_, arena_, local_state.count, source_count_);
        const blitzar_status synchronized_append_status =
            exchange_.SynchronizeStatus(append_status, "force-append");

        if (synchronized_append_status != BLITZAR_STATUS_OK) {
            return synchronized_append_status;
        }

        const blitzar_core::ParticleStateView full_state =
            SrvMakeArenaState(arena_, local_state.count, source_count_);
        blitzar_status remote_status = BLITZAR_STATUS_OK;

        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            if (source_count_ != local_state.count) {
                remote_status = base_.ComputeRange(
                    full_state, forces, settings, {local_state.count, source_count_, true});
            }
        }
        else if (!workspace.empty()) {
            remote_status = base_.Compute(full_state, forces, settings, workspace.front());
        }
        else {
            remote_status = base_.Compute(full_state, forces, settings);
        }

        return exchange_.SynchronizeStatus(remote_status, "force-remote");
    }

    SrvSolverDispatcher<Solver> base_;
    blitzar_parallel::MpiExchange& exchange_;
    blitzar_particles::ParticleArena& arena_;
    std::span<const std::uint64_t> ids_;
    blitzar_parallel::PacketBuffer& ghosts_;
    blitzar_parallel::MpiContext::GhostExchange& halo_;
    std::size_t& source_count_;
};

} // namespace blitzar_sdk

#endif
