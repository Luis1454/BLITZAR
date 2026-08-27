#include "mpi/exchange/MpiExchange.hpp"

#include <limits>

namespace blitzar_parallel {

blitzar_status MpiExchange::PrepareGatherCounts(int& local_count) const noexcept
{
    const std::size_t peer_count = static_cast<std::size_t>(context_.Size());

    if (state_.gather_counts.size() != peer_count ||
        state_.gather_displacements.size() != peer_count ||
        !ToCount(state_.local_packets.Size(), local_count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::fill(state_.gather_counts.begin(), state_.gather_counts.end(), 0);

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiExchange::PrepareGatherLayout(PacketBuffer& gathered) const noexcept
{
    std::size_t total = 0;

    std::fill(state_.gather_displacements.begin(), state_.gather_displacements.end(), 0);

    for (std::size_t index = 0; index < state_.gather_counts.size(); ++index) {
        const int count = state_.gather_counts[index];

        if (!ToCount(total, state_.gather_displacements[index]) || count < 0 ||
            total > std::numeric_limits<std::size_t>::max() - static_cast<std::size_t>(count)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        total += static_cast<std::size_t>(count);
    }

    return gathered.EnsureCapacity(total) && gathered.ResizeBounded(total)
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

} // namespace blitzar_parallel
