#include "mpi/runtime/MpiState.hpp"

namespace blitzar_parallel {

blitzar_status MpiContext::AllToAllCounts(
    std::span<const int> send_counts, std::span<int> receive_counts) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }

    return impl_->packets.AllToAllCounts(send_counts, receive_counts);
}

blitzar_status MpiContext::AllToAllPackets(const AllToAllPacketRequest& request) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }

    return impl_->packets.AllToAllPackets(request);
}

blitzar_status MpiContext::AllGatherCounts(int local_count, std::span<int> counts) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }

    return impl_->packets.AllGatherCounts(local_count, counts);
}

blitzar_status MpiContext::AllGatherPackets(std::span<const ParticlePacket> local_packets,
    std::span<ParticlePacket> gathered_packets, std::span<const int> counts,
    std::span<const int> displacements) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }

    return impl_->packets.AllGatherPackets(local_packets, gathered_packets, counts, displacements);
}

} // namespace blitzar_parallel
