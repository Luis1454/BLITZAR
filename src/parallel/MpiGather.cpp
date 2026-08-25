#include "parallel/MpiExchange.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace blitzar_parallel {

blitzar_status MpiExchange::Gather(blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids, PacketBuffer& gathered) const noexcept
{
    gathered.Clear();

    blitzar_status status = SynchronizeStatus(capacity_status_, "capacity");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = SynchronizeStatus(
        PackLocal(local_state, local_ids, state_.local_packets), "gather-pack");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    if (!context_.IsDistributed()) {
        if (!gathered.EnsureCapacity(state_.local_packets.Size()) ||
            !gathered.ResizeBounded(state_.local_packets.Size())) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        std::copy(state_.local_packets.View().begin(), state_.local_packets.View().end(),
            gathered.View().begin());

        return BLITZAR_STATUS_OK;
    }

    const int size = context_.Size();
    int local_count = 0;

    if (state_.gather_counts.size() != static_cast<std::size_t>(size) ||
        state_.gather_displacements.size() != static_cast<std::size_t>(size)) {
        return SynchronizeStatus(BLITZAR_STATUS_INVALID_ARGUMENT, "gather-capacity");
    }

    std::vector<int>& counts = state_.gather_counts;
    std::vector<int>& displacements = state_.gather_displacements;

    blitzar_status preparation_status = BLITZAR_STATUS_OK;

    if (!ToCount(state_.local_packets.Size(), local_count)) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::fill(counts.begin(), counts.end(), 0);

    status = SynchronizeStatus(preparation_status, "gather-count-prepare");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = context_.AllGatherCounts(local_count, counts);
    status = SynchronizeStatus(status, "gather-counts");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    std::size_t total = 0;

    preparation_status = BLITZAR_STATUS_OK;

    std::fill(displacements.begin(), displacements.end(), 0);

    for (int peer = 0; peer < size; ++peer) {
        const std::size_t index = static_cast<std::size_t>(peer);

        if (!ToCount(total, displacements[index]) || counts[index] < 0 ||
            total > std::numeric_limits<std::size_t>::max() -
                        static_cast<std::size_t>(counts[index])) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;

            break;
        }

        total += static_cast<std::size_t>(counts[index]);
    }

    if (preparation_status == BLITZAR_STATUS_OK) {
        if (!gathered.EnsureCapacity(total) || !gathered.ResizeBounded(total)) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    status = SynchronizeStatus(preparation_status, "gather-packet-prepare");

    if (status != BLITZAR_STATUS_OK) {
        gathered.Clear();

        return status;
    }

    status = context_.AllGatherPackets(
        state_.local_packets.View(), gathered.View(), counts, displacements);

    status = SynchronizeStatus(status, "gather-packets");

    if (status != BLITZAR_STATUS_OK) {
        gathered.Clear();
    }

    return status;
}

} // namespace blitzar_parallel
