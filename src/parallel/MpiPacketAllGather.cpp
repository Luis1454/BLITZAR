#include "parallel/MpiPacketTransport.hpp"

#include "parallel/MpiPacketProtocol.hpp"
#include "parallel/MpiSessionNative.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

bool MpiPacketTransport::ValidateAllGatherRequest(
    const AllGatherRequest& request, std::size_t peer_count, int rank) noexcept
{
    const bool layout_valid = request.counts.size() == peer_count &&
                              request.displacements.size() == peer_count &&
                              MpiPacketProtocol::ValidateLayout(
                                  request.counts, request.displacements,
                                  request.gathered_packets.size());

    return layout_valid && rank >= 0 && static_cast<std::size_t>(rank) < peer_count &&
           request.local_packets.size() ==
               static_cast<std::size_t>(request.counts[static_cast<std::size_t>(rank)]);
}

blitzar_status MpiPacketTransport::AllGatherPackets(
    std::span<const ParticlePacket> local_packets, std::span<ParticlePacket> gathered_packets,
    std::span<const int> counts, std::span<const int> displacements) const noexcept
{
    const AllGatherRequest request{local_packets, gathered_packets, counts, displacements};
    const std::size_t peer_count = static_cast<std::size_t>(session_.Size());

    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        if (!ValidateAllGatherRequest(request, peer_count, session_.Rank())) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        std::copy_n(request.local_packets.begin(), request.local_packets.size(),
            request.gathered_packets.begin() + request.displacements[0]);

        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)

    return RunAllGather(request);
#else

    (void)request;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

#if defined(BLITZAR_HAS_MPI)
blitzar_status MpiPacketTransport::RunAllGather(
    const AllGatherRequest& request) const noexcept
{
    std::size_t packets_per_peer = 0;
    blitzar_status status = PrepareAllGather(request, packets_per_peer);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    for (;;) {
        const int local_more = MpiPacketProtocol::HasRemaining(
            request.counts, send_progress_) ? 1 : 0;
        int global_more = 0;

        if (collectives_.ReduceMax(local_more, global_more) != BLITZAR_STATUS_OK) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        if (global_more == 0) {
            return BLITZAR_STATUS_OK;
        }

        PacketRoundLayout layout{};
        status = PrepareAllGatherRound(request, packets_per_peer, layout);

        if (status == BLITZAR_STATUS_OK) {
            status = EncodeAllGather(request, layout);
        }

        status = MpiPacketProtocol::SynchronizePreparation(
            collectives_, status, "allgather-packet-round-prepare");

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        if (MPI_Allgatherv(send_wire_.data(), layout.send_total_bytes, MPI_BYTE,
                receive_wire_.data(), receive_bytes_.data(), receive_offsets_.data(), MPI_BYTE,
                session_.Native().communicator) != MPI_SUCCESS) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        status = DecodeAllGather(request, layout);
        status = MpiPacketProtocol::SynchronizePreparation(
            collectives_, status, "allgather-packet-round-decode");

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        for (std::size_t index = 0; index < send_progress_.size(); ++index) {
            send_progress_[index] +=
                static_cast<std::size_t>(receive_bytes_[index] / ParticleWireBytes);
        }
    }
}
#endif

blitzar_status MpiPacketTransport::PrepareAllGather(
    const AllGatherRequest& request, std::size_t& packets_per_peer) const noexcept
{
    const std::size_t peer_count = static_cast<std::size_t>(session_.Size());
    blitzar_status status = ValidateAllGatherRequest(request, peer_count, session_.Rank())
                                ? BLITZAR_STATUS_OK
                                : BLITZAR_STATUS_INVALID_ARGUMENT;

    if (status == BLITZAR_STATUS_OK &&
        !MpiPacketProtocol::ComputeRoundPacketLimit(session_.Size(), packets_per_peer)) {
        status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (send_progress_.size() != peer_count || receive_bytes_.size() != peer_count ||
        receive_offsets_.size() != peer_count) {
        status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::fill(send_progress_.begin(), send_progress_.end(), 0);
    std::fill(receive_bytes_.begin(), receive_bytes_.end(), 0);
    std::fill(receive_offsets_.begin(), receive_offsets_.end(), 0);

    return MpiPacketProtocol::SynchronizePreparation(
        collectives_, status, "allgather-packet-prepare");
}

blitzar_status MpiPacketTransport::PrepareAllGatherRound(const AllGatherRequest& request,
    std::size_t packets_per_peer, PacketRoundLayout& layout) const noexcept
{
    layout = {};
    layout.local_index = static_cast<std::size_t>(session_.Rank());
    const std::size_t local_remaining =
        static_cast<std::size_t>(request.counts[layout.local_index]) -
        send_progress_[layout.local_index];
    layout.local_chunk = std::min(local_remaining, packets_per_peer);

    for (std::size_t index = 0; index < receive_bytes_.size(); ++index) {
        const std::size_t remaining =
            static_cast<std::size_t>(request.counts[index]) - send_progress_[index];
        const std::size_t chunk = std::min(remaining, packets_per_peer);

        if (!MpiPacketProtocol::ToWireBytes(layout.receive_total, receive_offsets_[index]) ||
            !MpiPacketProtocol::ToWireBytes(chunk, receive_bytes_[index]) ||
            layout.receive_total > std::numeric_limits<std::size_t>::max() - chunk) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        layout.receive_total += chunk;
    }

    if (!MpiPacketProtocol::ToWireBytes(layout.local_chunk, layout.send_total_bytes) ||
        !MpiPacketProtocol::ToWireBytes(layout.receive_total, layout.receive_total_bytes) ||
        !MpiPacketProtocol::ResizeWithinCapacity(
            send_wire_, static_cast<std::size_t>(layout.send_total_bytes)) ||
        !MpiPacketProtocol::ResizeWithinCapacity(
            receive_wire_, static_cast<std::size_t>(layout.receive_total_bytes))) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiPacketTransport::EncodeAllGather(
    const AllGatherRequest& request, const PacketRoundLayout& layout) const noexcept
{
    return ParticleWireCodec::Encode(
               request.local_packets.subspan(send_progress_[layout.local_index], layout.local_chunk),
               std::span<std::byte>(send_wire_.data(), send_wire_.size()))
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

blitzar_status MpiPacketTransport::DecodeAllGather(
    const AllGatherRequest& request, const PacketRoundLayout& layout) const noexcept
{
    for (std::size_t index = 0; index < receive_bytes_.size(); ++index) {
        const std::size_t chunk =
            static_cast<std::size_t>(receive_bytes_[index] / ParticleWireBytes);

        if (!ParticleWireCodec::Decode(
                std::span<const std::byte>(receive_wire_.data(), receive_wire_.size())
                    .subspan(static_cast<std::size_t>(receive_offsets_[index]),
                        static_cast<std::size_t>(receive_bytes_[index])),
                request.gathered_packets.subspan(
                    static_cast<std::size_t>(request.displacements[index]) +
                        send_progress_[index],
                    chunk))) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    (void)layout;

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_parallel
