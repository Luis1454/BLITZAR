#ifndef BLITZAR_SDK_DISPATCH_HPP
#define BLITZAR_SDK_DISPATCH_HPP

#include "core/Execution.hpp"
#include "core/Solver.hpp"
#include "gpu/HipContext.hpp"
#include "parallel/MpiExchange.hpp"
#include "parallel/MpiTrace.hpp"
#include "sdk/State.hpp"
#include "solvers/barnes_hut/BarnesHutSolver.hpp"
#include "solvers/barnes_hut/ThreadStackPool.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <atomic>
#include <chrono>
#include <blitzar/blitzar.h>
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

namespace blitzar_sdk {

template <typename Solver> struct SolverDispatchContext final {
    blitzar_gpu::HipContext& hip;
    Solver& cpu;
    blitzar_physics::GravityParameters gravity;
    blitzar_barnes_hut::BarnesHutSettings barnes_hut;
    std::atomic<blitzar_backend_kind>& backend;
};

template <typename Solver> class SolverDispatcher final {
public:
    explicit SolverDispatcher(SolverDispatchContext<Solver> context) noexcept
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
        blitzar_barnes_hut::ThreadStackPool& stack_pool) noexcept
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

            return cpu_.Compute(particles, forces, settings, stack_pool);
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
        if constexpr (std::is_same_v<Solver, blitzar_barnes_hut::BarnesHutSolver>) {
            backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

            return cpu_.ComputeSplit(request);
        }
        else {
            return BLITZAR_STATUS_UNSUPPORTED;
        }
    }

    [[nodiscard]] blitzar_status ComputeSplit(
        const blitzar_barnes_hut::BarnesHutSplitRequest& request,
        blitzar_barnes_hut::ThreadStackPool& stack_pool) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_barnes_hut::BarnesHutSolver>) {
            backend_.store(BLITZAR_BACKEND_CPU, std::memory_order_relaxed);

            return cpu_.ComputeSplit(request, stack_pool);
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

template <typename Solver> class DistributedDispatcher final {
public:
    struct State final {
        SolverDispatchContext<Solver> solver;
        blitzar_parallel::MpiExchange& exchange;
        blitzar_particles::SourceBuffer& sources;
        std::vector<std::uint64_t>& ids;
        blitzar_parallel::PacketBuffer& ghosts;
        blitzar_parallel::MpiContext::GhostExchange& halo;
        blitzar_parallel::MpiOverlapMode overlap_mode;
        blitzar_parallel::MpiOverlapTrace& overlap_trace;
    };

    explicit DistributedDispatcher(State state) noexcept
        : base_(state.solver), exchange_(state.exchange), sources_(state.sources), ids_(state.ids),
          ghosts_(state.ghosts), halo_(state.halo), overlap_mode_(state.overlap_mode),
          overlap_trace_(state.overlap_trace)
    {
    }

    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView local_state,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept
    {
        return ComputeWithOverlap(local_state, forces, settings, {});
    }

    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView local_state,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        blitzar_barnes_hut::ThreadStackPool& stack_pool) noexcept
    {
        return ComputeWithOverlap(local_state, forces, settings,
            std::span<blitzar_barnes_hut::ThreadStackPool>(&stack_pool, 1));
    }

    void Abort() noexcept
    {
        exchange_.AbortGhosts(halo_, ghosts_);

        (void)sources_.SetCount(0);
    }

private:
    using TraceClock = std::chrono::steady_clock;
    using TraceTime = TraceClock::time_point;

    [[nodiscard]] static std::uint64_t Elapsed(TraceTime start, TraceTime end) noexcept
    {
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    }

    [[nodiscard]] blitzar_status FinishTrace(
        blitzar_status status, TraceTime operation_start) noexcept
    {
        overlap_trace_.status = status;
        overlap_trace_.total_ns = Elapsed(operation_start, TraceClock::now());

        return status;
    }

    [[nodiscard]] blitzar_status ComputeLocal(blitzar_core::ParticleStateView local_state,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        std::span<blitzar_barnes_hut::ThreadStackPool> stack_pool) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            return base_.ComputeRange(local_state, forces, settings, {0, local_state.count, false});
        }
        else if (!stack_pool.empty()) {
            return base_.Compute(local_state, forces, settings, stack_pool.front());
        }
        else {
            return base_.Compute(local_state, forces, settings);
        }
    }

    [[nodiscard]] blitzar_status ComputeWithOverlap(blitzar_core::ParticleStateView local_state,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        std::span<blitzar_barnes_hut::ThreadStackPool> stack_pool) noexcept
    {
        const TraceTime operation_start = TraceClock::now();

        overlap_trace_.Reset(overlap_mode_);

        overlap_trace_.local_packets = local_state.count;

        const bool ids_valid = ids_.size() >= local_state.count;
        const std::span<const std::uint64_t> local_ids =
            ids_valid ? std::span<const std::uint64_t>(ids_).first(local_state.count)
                      : std::span<const std::uint64_t>{};

        const blitzar_status begin_status = exchange_.BeginGhosts(local_state, local_ids, halo_);

        overlap_trace_.begin_end_ns = Elapsed(operation_start, TraceClock::now());

        if (begin_status != BLITZAR_STATUS_OK) {
            return FinishTrace(begin_status, operation_start);
        }

        const blitzar_parallel::MpiGhostExchange::TransferStats transfer = halo_.Transfer();

        overlap_trace_.send_bytes = transfer.send_bytes;
        overlap_trace_.receive_bytes = transfer.receive_bytes;

        blitzar_status local_status = BLITZAR_STATUS_OK;
        blitzar_status complete_status = BLITZAR_STATUS_OK;

        if (overlap_mode_ == blitzar_parallel::MpiOverlapMode::Serialized) {
            overlap_trace_.complete_start_ns = Elapsed(operation_start, TraceClock::now());
            complete_status = exchange_.CompleteGhosts(halo_, ghosts_);
            overlap_trace_.complete_end_ns = Elapsed(operation_start, TraceClock::now());

            if (complete_status == BLITZAR_STATUS_OK) {
                overlap_trace_.local_start_ns = Elapsed(operation_start, TraceClock::now());
                local_status = ComputeLocal(local_state, forces, settings, stack_pool);
                overlap_trace_.local_end_ns = Elapsed(operation_start, TraceClock::now());
            }
        }
        else {
            overlap_trace_.local_start_ns = Elapsed(operation_start, TraceClock::now());
            local_status = ComputeLocal(local_state, forces, settings, stack_pool);
            overlap_trace_.local_end_ns = Elapsed(operation_start, TraceClock::now());

            overlap_trace_.complete_start_ns = Elapsed(operation_start, TraceClock::now());
            complete_status = exchange_.CompleteGhosts(halo_, ghosts_);
            overlap_trace_.complete_end_ns = Elapsed(operation_start, TraceClock::now());
        }

        if (complete_status != BLITZAR_STATUS_OK) {
            return FinishTrace(complete_status, operation_start);
        }

        overlap_trace_.ghost_packets = ghosts_.Size();

        const blitzar_status synchronized_local_status =
            exchange_.SynchronizeStatus(local_status, "force-local");

        if (synchronized_local_status != BLITZAR_STATUS_OK) {
            return FinishTrace(synchronized_local_status, operation_start);
        }

        const blitzar_status append_status = StoreGhosts(ghosts_, sources_);

        const blitzar_status synchronized_append_status =
            exchange_.SynchronizeStatus(append_status, "force-store");

        if (synchronized_append_status != BLITZAR_STATUS_OK) {
            return FinishTrace(synchronized_append_status, operation_start);
        }

        const blitzar_core::ParticleStateView remote_state = sources_.State();

        overlap_trace_.remote_start_ns = Elapsed(operation_start, TraceClock::now());

        blitzar_status remote_status = BLITZAR_STATUS_OK;

        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            remote_status = base_.ComputeRemote(local_state, remote_state, forces, settings);
        }
        else if (!stack_pool.empty()) {
            remote_status = base_.ComputeSplit(
                {local_state, remote_state, forces, settings}, stack_pool.front());
        }
        else {
            remote_status = base_.ComputeSplit({local_state, remote_state, forces, settings});
        }

        overlap_trace_.remote_end_ns = Elapsed(operation_start, TraceClock::now());

        const blitzar_status synchronized_remote_status =
            exchange_.SynchronizeStatus(remote_status, "force-remote");

        if (synchronized_remote_status == BLITZAR_STATUS_OK) {
            (void)sources_.SetCount(0);
        }

        return FinishTrace(synchronized_remote_status, operation_start);
    }

    SolverDispatcher<Solver> base_;
    blitzar_parallel::MpiExchange& exchange_;
    blitzar_particles::SourceBuffer& sources_;
    std::vector<std::uint64_t>& ids_;
    blitzar_parallel::PacketBuffer& ghosts_;
    blitzar_parallel::MpiContext::GhostExchange& halo_;
    blitzar_parallel::MpiOverlapMode overlap_mode_;
    blitzar_parallel::MpiOverlapTrace& overlap_trace_;
};

} // namespace blitzar_sdk

#endif
