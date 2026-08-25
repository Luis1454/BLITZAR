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
    [[nodiscard]] blitzar_status AllToAllPackets(
        const AllToAllPacketRequest& request) const noexcept;
    [[nodiscard]] blitzar_status AllGatherCounts(
        int local_count, std::span<int> counts) const noexcept;
    [[nodiscard]] blitzar_status AllGatherPackets(std::span<const ParticlePacket> local_packets,
        std::span<ParticlePacket> gathered_packets, std::span<const int> counts,
        std::span<const int> displacements) const noexcept;

private:
    struct PacketRoundLayout final {
        std::size_t send_total{};
        std::size_t receive_total{};
        int send_total_bytes{};
        int receive_total_bytes{};
        std::size_t local_index{};
        std::size_t local_chunk{};
    };

    struct AllGatherRequest final {
        std::span<const ParticlePacket> local_packets;
        std::span<ParticlePacket> gathered_packets;
        std::span<const int> counts;
        std::span<const int> displacements;
    };

    [[nodiscard]] static bool ValidateAllToAllRequest(
        const AllToAllPacketRequest& request, std::size_t peer_count) noexcept;
    [[nodiscard]] blitzar_status ExecuteAllToAllCounts(bool layout_valid,
        std::span<const int> send_counts, std::span<int> receive_counts) const noexcept;
    [[nodiscard]] blitzar_status ExecuteAllGatherCounts(bool layout_valid,
        int local_count, std::span<int> counts) const noexcept;
#if defined(BLITZAR_HAS_MPI)
    [[nodiscard]] blitzar_status RunAllToAll(
        const AllToAllPacketRequest& request) const noexcept;
#endif
    [[nodiscard]] blitzar_status PrepareAllToAll(
        const AllToAllPacketRequest& request, std::size_t& packets_per_peer) const noexcept;
    [[nodiscard]] blitzar_status PrepareAllToAllRound(const AllToAllPacketRequest& request,
        std::size_t packets_per_peer, PacketRoundLayout& layout) const noexcept;
    [[nodiscard]] blitzar_status EncodeAllToAll(
        const AllToAllPacketRequest& request) const noexcept;
    [[nodiscard]] blitzar_status DecodeAllToAll(
        const AllToAllPacketRequest& request) const noexcept;
    [[nodiscard]] static bool ValidateAllGatherRequest(
        const AllGatherRequest& request, std::size_t peer_count, int rank) noexcept;
#if defined(BLITZAR_HAS_MPI)
    [[nodiscard]] blitzar_status RunAllGather(
        const AllGatherRequest& request) const noexcept;
#endif
    [[nodiscard]] blitzar_status PrepareAllGather(
        const AllGatherRequest& request, std::size_t& packets_per_peer) const noexcept;
    [[nodiscard]] blitzar_status PrepareAllGatherRound(const AllGatherRequest& request,
        std::size_t packets_per_peer, PacketRoundLayout& layout) const noexcept;
    [[nodiscard]] blitzar_status EncodeAllGather(
        const AllGatherRequest& request, const PacketRoundLayout& layout) const noexcept;
    [[nodiscard]] blitzar_status DecodeAllGather(
        const AllGatherRequest& request, const PacketRoundLayout& layout) const noexcept;

    const MpiSession& session_;
    [[maybe_unused]] const MpiCollectives& collectives_;
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
