#include "parallel/MpiExchange.hpp"

#include <algorithm>

namespace blitzar_parallel {

blitzar_status MpiExchange::GatherLocal(PacketBuffer& gathered) const noexcept
{
    if (!gathered.EnsureCapacity(state_.local_packets.Size()) ||
        !gathered.ResizeBounded(state_.local_packets.Size())) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::copy(state_.local_packets.View().begin(), state_.local_packets.View().end(),
        gathered.View().begin());

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_parallel
