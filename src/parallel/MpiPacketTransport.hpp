#ifndef BLITZAR_PARALLEL_MPI_PACKET_TRANSPORT_HPP
#define BLITZAR_PARALLEL_MPI_PACKET_TRANSPORT_HPP

#include "parallel/MpiCollectives.hpp"
#include "parallel/MpiTypes.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <span>
#include <vector>

namespace blitzar_parallel {

class MpiPacketTransport final {
public:
    MpiPacketTransport(const MpiSession& session, const MpiCollectives& collectives) noexcept;

    [[nodiscard]] blitzar_status Prepare(std::size_t packet_capacity) noexcept;

    [[nodiscard]] blitzar_status AllToAllCounts(
        std::span<const int> send_counts, std::span<int> receive_counts) const noexcept;
    [[nodiscard]] blitzar_status AllToAllPackets(std::span<const ParticlePacket> send_packets,
        std::span<const int> send_counts, std::span<const int> send_displacements,
        std::span<ParticlePacket> receive_packets, std::span<const int> receive_counts,
        std::span<const int> receive_displacements) const noexcept;
    [[nodiscard]] blitzar_status AllGatherCounts(
        int local_count, std::span<int> counts) const noexcept;
    [[nodiscard]] blitzar_status AllGatherPackets(std::span<const ParticlePacket> local_packets,
        std::span<ParticlePacket> gathered_packets, std::span<const int> counts,
        std::span<const int> displacements) const noexcept;

private:
    const MpiSession& session_;
    const MpiCollectives& collectives_;
    std::size_t packet_capacity_{0};
    mutable std::vector<std::size_t> send_progress_;
    mutable std::vector<std::size_t> receive_progress_;
    mutable std::vector<int> send_bytes_;
    mutable std::vector<int> receive_bytes_;
    mutable std::vector<int> send_offsets_;
    mutable std::vector<int> receive_offsets_;
    mutable std::vector<std::byte> send_wire_;
    mutable std::vector<std::byte> receive_wire_;
};

} // namespace blitzar_parallel

#endif
