#include "MpiCases.hpp"
#include "fixtures/AllocationMonitor.hpp"
#include "fixtures/Views.hpp"

#include <cstdio>

namespace blitzar_mpi_tests {

namespace {

struct OverlapObservation final {
    StateArrays overlapped_state{};
    StateArrays serialized_state{};
    blitzar_parallel::MpiOverlapTrace overlapped_trace{};
    blitzar_parallel::MpiOverlapTrace serialized_trace{};
};

bool ConfigureOverlap(blitzar_sim::Simulation& overlapped, blitzar_sim::Simulation& serialized,
    const StateArrays& initial) noexcept
{
    if (!Configure(overlapped, initial, 0.01)) {
        return false;
    }

    if (!Configure(serialized, initial, 0.01)) {
        return false;
    }

    overlapped.SetMpiOverlapForTesting(blitzar_parallel::MpiOverlapMode::Overlapped);
    serialized.SetMpiOverlapForTesting(blitzar_parallel::MpiOverlapMode::Serialized);

    return true;
}

bool AdvanceOverlap(
    blitzar_sim::Simulation& overlapped, blitzar_sim::Simulation& serialized) noexcept
{
    if (overlapped.Step() != BLITZAR_STATUS_OK) {
        return false;
    }

    return serialized.Step() == BLITZAR_STATUS_OK;
}

bool CaptureOverlapState(blitzar_sim::Simulation& overlapped, blitzar_sim::Simulation& serialized,
    OverlapObservation& observation) noexcept
{
    if (overlapped.GetState(blitzar_tests::MakeOutputView(observation.overlapped_state)) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    return serialized.GetState(blitzar_tests::MakeOutputView(observation.serialized_state)) ==
           BLITZAR_STATUS_OK;
}

bool CaptureSteadyState(blitzar_sim::Simulation& overlapped, blitzar_sim::Simulation& serialized,
    OverlapObservation& observation) noexcept
{
    blitzar_tests::BeginAllocationCounting();

    const blitzar_status overlapped_status = overlapped.Step();
    const blitzar_status serialized_status = serialized.Step();
    const blitzar_status overlapped_state_status =
        overlapped.GetState(blitzar_tests::MakeOutputView(observation.overlapped_state));

    const blitzar_status serialized_state_status =
        serialized.GetState(blitzar_tests::MakeOutputView(observation.serialized_state));

    const std::size_t allocations = blitzar_tests::EndAllocationCounting();

    if (overlapped_status != BLITZAR_STATUS_OK || serialized_status != BLITZAR_STATUS_OK) {
        return false;
    }

    if (overlapped_state_status != BLITZAR_STATUS_OK ||
        serialized_state_status != BLITZAR_STATUS_OK) {
        return false;
    }

    if (allocations != 0) {
        return false;
    }

    observation.overlapped_trace = overlapped.LastMpiOverlapTrace();
    observation.serialized_trace = serialized.LastMpiOverlapTrace();

    return true;
}

bool HasMatchingVolume(const OverlapObservation& observation) noexcept
{
    const auto& overlapped = observation.overlapped_trace;
    const auto& serialized = observation.serialized_trace;

    return overlapped.local_packets == serialized.local_packets &&
           overlapped.ghost_packets == serialized.ghost_packets &&
           overlapped.send_bytes == serialized.send_bytes &&
           overlapped.receive_bytes == serialized.receive_bytes;
}

bool HasValidTimeline(const OverlapObservation& observation) noexcept
{
    const auto& overlapped = observation.overlapped_trace;
    const auto& serialized = observation.serialized_trace;

    return overlapped.status == BLITZAR_STATUS_OK && serialized.status == BLITZAR_STATUS_OK &&
           overlapped.HasOverlap() && !serialized.HasOverlap() && overlapped.total_ns > 0 &&
           serialized.total_ns > 0;
}

bool ReportOverlap(
    const blitzar_parallel::MpiContext& context, const OverlapObservation& observation) noexcept
{
    const auto& overlapped = observation.overlapped_trace;
    const auto& serialized = observation.serialized_trace;

    const bool parity = StatesMatch(observation.overlapped_state, observation.serialized_state);
    const bool volume_match = HasMatchingVolume(observation);
    const bool timeline_valid = HasValidTimeline(observation);
    const double speedup =
        static_cast<double>(serialized.total_ns) / static_cast<double>(overlapped.total_ns);

    if (context.Rank() == 0) {
        std::fprintf(stdout,
            "BLITZAR MPI overlap ranks=%d local_packets=%zu ghost_packets=%zu "
            "send_bytes=%zu receive_bytes=%zu serialized_ns=%llu overlapped_ns=%llu "
            "speedup=%.6f parity=%d\n",
            context.Size(), overlapped.local_packets, overlapped.ghost_packets,
            overlapped.send_bytes, overlapped.receive_bytes,
            static_cast<unsigned long long>(serialized.total_ns),
            static_cast<unsigned long long>(overlapped.total_ns), speedup, parity ? 1 : 0);
    }

    return parity && volume_match && timeline_valid;
}

} // namespace

bool RunOverlapCase(blitzar_parallel::MpiContext& context) noexcept
{
    const StateArrays initial = InitialState();
    blitzar_sim::Simulation overlapped(ParticleCount);
    blitzar_sim::Simulation serialized(ParticleCount);
    OverlapObservation observation{};

    if (!ConfigureOverlap(overlapped, serialized, initial) ||
        !AdvanceOverlap(overlapped, serialized) ||
        !CaptureOverlapState(overlapped, serialized, observation) ||
        !CaptureSteadyState(overlapped, serialized, observation)) {
        return false;
    }

    return ReportOverlap(context, observation);
}

} // namespace blitzar_mpi_tests
