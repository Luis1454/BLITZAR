#include "parallel/mpi/exchange/MpiExchange.hpp"

namespace blitzar_parallel {

blitzar_status MpiExchange::GatherPackets(PacketBuffer& gathered) const noexcept
{
    const blitzar_status status = context_.AllGatherPackets(state_.local_packets.View(),
        gathered.View(), state_.gather_counts, state_.gather_displacements);

    const blitzar_status synchronized_status = SynchronizeStatus(status, "gather-packets");

    if (synchronized_status != BLITZAR_STATUS_OK) {
        gathered.Clear();
    }

    return synchronized_status;
}

} // namespace blitzar_parallel
