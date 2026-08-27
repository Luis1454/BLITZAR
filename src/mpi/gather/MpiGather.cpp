#include "mpi/exchange/MpiExchange.hpp"

namespace blitzar_parallel {

blitzar_status MpiExchange::Gather(blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids, PacketBuffer& gathered) const noexcept
{
    gathered.Clear();

    blitzar_status status = SynchronizeStatus(capacity_status_, "capacity");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status =
        SynchronizeStatus(PackLocal(local_state, local_ids, state_.local_packets), "gather-pack");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    if (!context_.IsDistributed()) {
        return GatherLocal(gathered);
    }

    int local_count = 0;

    status = SynchronizeStatus(PrepareGatherCounts(local_count), "gather-count-prepare");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = context_.AllGatherCounts(local_count, state_.gather_counts);
    status = SynchronizeStatus(status, "gather-counts");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = SynchronizeStatus(PrepareGatherLayout(gathered), "gather-packet-prepare");

    if (status != BLITZAR_STATUS_OK) {
        gathered.Clear();

        return status;
    }

    return GatherPackets(gathered);
}

} // namespace blitzar_parallel
