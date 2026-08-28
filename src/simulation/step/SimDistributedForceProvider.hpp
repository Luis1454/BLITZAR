#ifndef BLITZAR_SIMULATION_STEP_SIM_DISTRIBUTED_FORCE_PROVIDER_HPP
#define BLITZAR_SIMULATION_STEP_SIM_DISTRIBUTED_FORCE_PROVIDER_HPP

#include "mpi/exchange/MpiExchange.hpp"
#include "mpi/exchange/MpiExchangeTrace.hpp"
#include "simulation/solver/SimBackendForceProvider.hpp"
#include "simulation/step/SimOverlapClock.hpp"
#include "simulation/step/SimPacketStoreRequest.hpp"

#include <blitzar/blitzar.h>
#include <cstdint>
#include <span>
#include <vector>

namespace blitzar_sim {

template <typename Solver> class SimDistributedForceProvider final {
public:
    struct ProviderContext final {
        SimBackendForceContext<Solver> backend;
        blitzar_parallel::MpiExchange& exchange;
        blitzar_particles::ParticleSourceBuffer& sources;
        std::vector<std::uint64_t>& ids;
        blitzar_parallel::PacketBuffer& ghosts;
        blitzar_parallel::MpiContext::GhostExchange& halo;
        blitzar_parallel::MpiOverlapMode overlap_mode;
        blitzar_parallel::MpiOverlapTrace& overlap_trace;
    };

    explicit SimDistributedForceProvider(ProviderContext context) noexcept
        : base_(context.backend), exchange_(context.exchange), sources_(context.sources),
          ids_(context.ids), ghosts_(context.ghosts), halo_(context.halo),
          overlap_mode_(context.overlap_mode), overlap_trace_(context.overlap_trace)
    {
    }

    [[nodiscard]] blitzar_status Evaluate(
        const blitzar_solvers::SolverForceEvaluation& request) noexcept
    {
        if (request.source_kind != blitzar_solvers::SolverForceSourceKind::Local) {
            return BLITZAR_STATUS_UNSUPPORTED;
        }

        return RunOverlap(request);
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
        blitzar_solvers::SolverForceEvaluation evaluation;
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

        overlap_trace_.local_packets = request.evaluation.targets.count;

        const bool ids_valid = ids_.size() >= request.evaluation.targets.count;
        const std::span<const std::uint64_t> local_ids =
            ids_valid ? std::span<const std::uint64_t>(ids_).first(request.evaluation.targets.count)
                      : std::span<const std::uint64_t>{};

        const blitzar_status status =
            exchange_.BeginGhosts(request.evaluation.targets, local_ids, halo_);

        overlap_trace_.begin_end_ns =
            SimOverlapClock::Elapsed(request.operation_start, TraceClock::now());

        return status;
    }

    [[nodiscard]] blitzar_status EvaluateLocal(const OverlapRequest& request) noexcept
    {
        const blitzar_solvers::SolverForceEvaluation local_request{request.evaluation.targets,
            request.evaluation.targets, request.evaluation.forces, request.evaluation.settings,
            blitzar_solvers::SolverForceSourceKind::Local};

        return base_.Evaluate(local_request);
    }

    [[nodiscard]] blitzar_status RunLocal(const OverlapRequest& request) noexcept
    {
        overlap_trace_.local_start_ns =
            SimOverlapClock::Elapsed(request.operation_start, TraceClock::now());

        const blitzar_status status = EvaluateLocal(request);

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

    [[nodiscard]] blitzar_status EvaluateRemote(const OverlapRequest& request) noexcept
    {
        const blitzar_core::ParticleStateView remote_state = sources_.State();

        overlap_trace_.remote_start_ns =
            SimOverlapClock::Elapsed(request.operation_start, TraceClock::now());

        if (remote_state.SourceCount() == 0) {
            overlap_trace_.remote_end_ns =
                SimOverlapClock::Elapsed(request.operation_start, TraceClock::now());

            return BLITZAR_STATUS_OK;
        }

        const blitzar_solvers::SolverForceEvaluation remote_request{request.evaluation.targets,
            remote_state, request.evaluation.forces, request.evaluation.settings,
            blitzar_solvers::SolverForceSourceKind::Remote};

        const blitzar_status status = base_.Evaluate(remote_request);

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

        const blitzar_status remote_status = EvaluateRemote(request);
        const blitzar_status synchronized_remote_status =
            exchange_.SynchronizeStatus(remote_status, "force-remote");

        if (synchronized_remote_status == BLITZAR_STATUS_OK) {
            (void)sources_.SetCount(0);
        }

        return FinishTrace(synchronized_remote_status, request.operation_start);
    }

    [[nodiscard]] blitzar_status RunOverlap(
        const blitzar_solvers::SolverForceEvaluation& evaluation) noexcept
    {
        const OverlapRequest request{evaluation, TraceClock::now()};

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

    SimBackendForceProvider<Solver> base_;
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
