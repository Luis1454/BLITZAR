#ifndef BLITZAR_SIMULATION_STEP_SIM_OVERLAP_DISPATCH_HPP
#define BLITZAR_SIMULATION_STEP_SIM_OVERLAP_DISPATCH_HPP

#include "mpi/exchange/MpiExchange.hpp"
#include "mpi/exchange/MpiExchangeTrace.hpp"
#include "simulation/solver/SimSolverDispatch.hpp"
#include "simulation/step/SimOverlapClock.hpp"
#include "simulation/step/SimPacketStoreRequest.hpp"

#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

namespace blitzar_sim {

template <typename Solver> class DistributedDispatcher final {
public:
    struct DispatchContext final {
        SolverDispatchContext<Solver> solver;
        blitzar_parallel::MpiExchange& exchange;
        blitzar_particles::ParticleSourceBuffer& sources;
        std::vector<std::uint64_t>& ids;
        blitzar_parallel::PacketBuffer& ghosts;
        blitzar_parallel::MpiContext::GhostExchange& halo;
        blitzar_parallel::MpiOverlapMode overlap_mode;
        blitzar_parallel::MpiOverlapTrace& overlap_trace;
    };

    explicit DistributedDispatcher(DispatchContext state) noexcept
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
        blitzar_solver_threading::ThreadStackPool& stack_pool) noexcept
    {
        return ComputeWithOverlap(local_state, forces, settings,
            std::span<blitzar_solver_threading::ThreadStackPool>(&stack_pool, 1));
    }

    void Abort() noexcept
    {
        exchange_.AbortGhosts(halo_, ghosts_);

        (void)sources_.SetCount(0);
    }

private:
    using TraceClock = SimOverlapClock::Clock;
    using TraceTime = SimOverlapClock::Time;

    struct OverlapRequest final {
        blitzar_core::ParticleStateView local_state;
        blitzar_core::ForceView forces;
        const blitzar_core::ExecutionSettings& settings;
        std::span<blitzar_solver_threading::ThreadStackPool> stack_pool;
        TraceTime operation_start;
    };

    [[nodiscard]] blitzar_status FinishTrace(
        blitzar_status status, TraceTime operation_start) noexcept
    {
        overlap_trace_.status = status;
        overlap_trace_.total_ns = SimOverlapClock::Elapsed(operation_start, TraceClock::now());

        return status;
    }

    [[nodiscard]] blitzar_status BeginOverlap(const OverlapRequest& request) noexcept
    {
        overlap_trace_.Reset(overlap_mode_);

        overlap_trace_.local_packets = request.local_state.count;

        const bool ids_valid = ids_.size() >= request.local_state.count;
        const std::span<const std::uint64_t> local_ids =
            ids_valid ? std::span<const std::uint64_t>(ids_).first(request.local_state.count)
                      : std::span<const std::uint64_t>{};

        const blitzar_status status = exchange_.BeginGhosts(request.local_state, local_ids, halo_);

        overlap_trace_.begin_end_ns =
            SimOverlapClock::Elapsed(request.operation_start, TraceClock::now());

        return status;
    }

    [[nodiscard]] blitzar_status ComputeLocal(const OverlapRequest& request) noexcept
    {
        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            return base_.ComputeRange(request.local_state, request.forces, request.settings,
                {0, request.local_state.count, false});
        }
        else if constexpr (std::is_same_v<Solver, blitzar_fmm::FmmSolver>) {
            return base_.Compute(request.local_state, request.forces, request.settings);
        }
        else if (!request.stack_pool.empty()) {
            return base_.Compute(
                request.local_state, request.forces, request.settings, request.stack_pool.front());
        }
        else {
            return base_.Compute(request.local_state, request.forces, request.settings);
        }
    }

    [[nodiscard]] blitzar_status RunLocal(const OverlapRequest& request) noexcept
    {
        overlap_trace_.local_start_ns =
            SimOverlapClock::Elapsed(request.operation_start, TraceClock::now());

        const blitzar_status status = ComputeLocal(request);

        overlap_trace_.local_end_ns =
            SimOverlapClock::Elapsed(request.operation_start, TraceClock::now());

        return status;
    }

    [[nodiscard]] blitzar_status CompleteGhosts(const OverlapRequest& request) noexcept
    {
        overlap_trace_.complete_start_ns =
            SimOverlapClock::Elapsed(request.operation_start, TraceClock::now());

        const blitzar_status status = exchange_.CompleteGhosts(halo_, ghosts_);

        overlap_trace_.complete_end_ns =
            SimOverlapClock::Elapsed(request.operation_start, TraceClock::now());

        return status;
    }

    [[nodiscard]] blitzar_status ComputeRemote(const OverlapRequest& request) noexcept
    {
        const blitzar_core::ParticleStateView remote_state = sources_.State();

        overlap_trace_.remote_start_ns =
            SimOverlapClock::Elapsed(request.operation_start, TraceClock::now());

        blitzar_status status = BLITZAR_STATUS_OK;

        if constexpr (std::is_same_v<Solver, blitzar_direct::DirectSolver>) {
            status = base_.ComputeRemote(
                request.local_state, remote_state, request.forces, request.settings);
        }
        else if (!request.stack_pool.empty()) {
            status = base_.ComputeSplit(
                {request.local_state, remote_state, request.forces, request.settings},
                request.stack_pool.front());
        }
        else {
            status = base_.ComputeSplit(
                {request.local_state, remote_state, request.forces, request.settings});
        }

        overlap_trace_.remote_end_ns =
            SimOverlapClock::Elapsed(request.operation_start, TraceClock::now());

        return status;
    }

    [[nodiscard]] blitzar_status FinishRemote(const OverlapRequest& request,
        blitzar_status local_status, blitzar_status complete_status) noexcept
    {
        if (complete_status != BLITZAR_STATUS_OK) {
            return FinishTrace(complete_status, request.operation_start);
        }

        overlap_trace_.ghost_packets = ghosts_.Size();

        const blitzar_status synchronized_local_status =
            exchange_.SynchronizeStatus(local_status, "force-local");

        if (synchronized_local_status != BLITZAR_STATUS_OK) {
            return FinishTrace(synchronized_local_status, request.operation_start);
        }

        const blitzar_status append_status = StoreGhosts(ghosts_, sources_);
        const blitzar_status synchronized_append_status =
            exchange_.SynchronizeStatus(append_status, "force-store");

        if (synchronized_append_status != BLITZAR_STATUS_OK) {
            return FinishTrace(synchronized_append_status, request.operation_start);
        }

        const blitzar_status remote_status = ComputeRemote(request);
        const blitzar_status synchronized_remote_status =
            exchange_.SynchronizeStatus(remote_status, "force-remote");

        if (synchronized_remote_status == BLITZAR_STATUS_OK) {
            (void)sources_.SetCount(0);
        }

        return FinishTrace(synchronized_remote_status, request.operation_start);
    }

    [[nodiscard]] blitzar_status ComputeWithOverlap(blitzar_core::ParticleStateView local_state,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        std::span<blitzar_solver_threading::ThreadStackPool> stack_pool) noexcept
    {
        const OverlapRequest request{local_state, forces, settings, stack_pool, TraceClock::now()};

        const blitzar_status begin_status = BeginOverlap(request);

        if (begin_status != BLITZAR_STATUS_OK) {
            return FinishTrace(begin_status, request.operation_start);
        }

        const blitzar_parallel::MpiGhostExchange::TransferStats transfer = halo_.Transfer();

        overlap_trace_.send_bytes = transfer.send_bytes;
        overlap_trace_.receive_bytes = transfer.receive_bytes;

        blitzar_status local_status = BLITZAR_STATUS_OK;
        blitzar_status complete_status = BLITZAR_STATUS_OK;

        if (overlap_mode_ == blitzar_parallel::MpiOverlapMode::Serialized) {
            complete_status = CompleteGhosts(request);

            if (complete_status == BLITZAR_STATUS_OK) {
                local_status = RunLocal(request);
            }
        }
        else {
            local_status = RunLocal(request);
            complete_status = CompleteGhosts(request);
        }

        return FinishRemote(request, local_status, complete_status);
    }

    SolverDispatcher<Solver> base_;
    blitzar_parallel::MpiExchange& exchange_;
    blitzar_particles::ParticleSourceBuffer& sources_;
    std::vector<std::uint64_t>& ids_;
    blitzar_parallel::PacketBuffer& ghosts_;
    blitzar_parallel::MpiContext::GhostExchange& halo_;
    blitzar_parallel::MpiOverlapMode overlap_mode_;
    blitzar_parallel::MpiOverlapTrace& overlap_trace_;
};

} // namespace blitzar_sim

#endif
