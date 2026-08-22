#ifndef BLITZAR_PARALLEL_MPI_PACKET_TRANSPORT_HPP
#define BLITZAR_PARALLEL_MPI_PACKET_TRANSPORT_HPP

#include "parallel/MpiCollectives.hpp"
#include "parallel/MpiTypes.hpp"

#include <blitzar/blitzar.h>

#include <span>

namespace blitzar_parallel {

class MpiPacketTransport final {
public:
    MpiPacketTransport(
        const MpiSession& session,
        const MpiCollectives& collectives) noexcept;

    [[nodiscard]] blitzar_status AllToAllCounts(
        std::span<const int> send_counts,
        std::span<int> receive_counts) const noexcept;
    [[nodiscard]] blitzar_status AllToAllPackets(
        std::span<const ParticlePacket> send_packets,
        std::span<const int> send_counts,
        std::span<const int> send_displacements,
        std::span<ParticlePacket> receive_packets,
        std::span<const int> receive_counts,
        std::span<const int> receive_displacements) const noexcept;
    [[nodiscard]] blitzar_status AllGatherCounts(
        int local_count, std::span<int> counts) const noexcept;
    [[nodiscard]] blitzar_status AllGatherPackets(
        std::span<const ParticlePacket> local_packets,
        std::span<ParticlePacket> gathered_packets,
        std::span<const int> counts,
        std::span<const int> displacements) const noexcept;

private:
    const MpiSession& session_;
    const MpiCollectives& collectives_;
};

}  // namespace blitzar_parallel

#endif
