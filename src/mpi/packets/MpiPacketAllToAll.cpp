#include "mpi/packets/MpiPacketProtocol.hpp"
#include "mpi/packets/MpiPacketTransport.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace blitzar_parallel {

bool MpiPacketTransport::ValidateAllToAllRequest(
    const AllToAllPacketRequest& request, std::size_t peer_count) noexcept
{
    return request.send_counts.size() == peer_count &&
           request.send_displacements.size() == peer_count &&
           request.receive_counts.size() == peer_count &&
           request.receive_displacements.size() == peer_count &&
           MpiPacketProtocol::ValidateLayout(
               request.send_counts, request.send_displacements, request.send_packets.size()) &&
           MpiPacketProtocol::ValidateLayout(request.receive_counts, request.receive_displacements,
               request.receive_packets.size());
}

blitzar_status MpiPacketTransport::AllToAllPackets(
    const AllToAllPacketRequest& request) const noexcept
{
    const std::size_t peer_count = static_cast<std::size_t>(session_.Size());
    const bool layout_valid = ValidateAllToAllRequest(request, peer_count);

    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        if (!layout_valid || request.send_counts[0] != request.receive_counts[0]) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        std::copy_n(request.send_packets.begin() + request.send_displacements[0],
            static_cast<std::size_t>(request.send_counts[0]),
            request.receive_packets.begin() + request.receive_displacements[0]);

        return BLITZAR_STATUS_OK;
    }

    return RunAllToAll(request);
}

blitzar_status MpiPacketTransport::RunAllToAll(const AllToAllPacketRequest& request) const noexcept
{
    std::size_t packets_per_peer = 0;
    blitzar_status status = PrepareAllToAll(request, packets_per_peer);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    for (;;) {
        const int local_more =
            MpiPacketProtocol::HasRemaining(request.send_counts, send_progress_) ||
                    MpiPacketProtocol::HasRemaining(request.receive_counts, receive_progress_)
                ? 1
                : 0;

        int global_more = 0;

        if (collectives_.ReduceMax(local_more, global_more) != BLITZAR_STATUS_OK) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        if (global_more == 0) {
            return BLITZAR_STATUS_OK;
        }

        PacketRoundLayout layout{};

        status = PrepareAllToAllRound(request, packets_per_peer, layout);

        if (status == BLITZAR_STATUS_OK) {
            status = EncodeAllToAll(request);
        }

        status = MpiPacketProtocol::SynchronizePreparation(
            collectives_, status, "alltoall-packet-round-prepare");

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        const NativeByteAllToAllRequest native_request{
            std::span<const std::byte>(send_wire_.data(), send_wire_.size()), send_bytes_,
            send_offsets_, std::span<std::byte>(receive_wire_.data(), receive_wire_.size()),
            receive_bytes_, receive_offsets_};

        if (session_.Native().AllToAllBytes(native_request) != BLITZAR_STATUS_OK) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        status = DecodeAllToAll(request);
        status = MpiPacketProtocol::SynchronizePreparation(
            collectives_, status, "alltoall-packet-round-decode");

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        for (std::size_t index = 0; index < send_progress_.size(); ++index) {
            send_progress_[index] +=
                static_cast<std::size_t>(send_bytes_[index] / ParticleWireBytes);

            receive_progress_[index] +=
                static_cast<std::size_t>(receive_bytes_[index] / ParticleWireBytes);
        }
    }
}

blitzar_status MpiPacketTransport::PrepareAllToAll(
    const AllToAllPacketRequest& request, std::size_t& packets_per_peer) const noexcept
{
    const std::size_t peer_count = static_cast<std::size_t>(session_.Size());
    blitzar_status status = ValidateAllToAllRequest(request, peer_count)
                                ? BLITZAR_STATUS_OK
                                : BLITZAR_STATUS_INVALID_ARGUMENT;

    if (status == BLITZAR_STATUS_OK &&
        !MpiPacketProtocol::ComputeRoundPacketLimit(session_.Size(), packets_per_peer)) {
        status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (send_progress_.size() != peer_count || receive_progress_.size() != peer_count ||
        send_bytes_.size() != peer_count || receive_bytes_.size() != peer_count ||
        send_offsets_.size() != peer_count || receive_offsets_.size() != peer_count) {
        status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::fill(send_progress_.begin(), send_progress_.end(), 0);
    std::fill(receive_progress_.begin(), receive_progress_.end(), 0);
    std::fill(send_bytes_.begin(), send_bytes_.end(), 0);
    std::fill(receive_bytes_.begin(), receive_bytes_.end(), 0);
    std::fill(send_offsets_.begin(), send_offsets_.end(), 0);
    std::fill(receive_offsets_.begin(), receive_offsets_.end(), 0);

    return MpiPacketProtocol::SynchronizePreparation(
        collectives_, status, "alltoall-packet-prepare");
}

blitzar_status MpiPacketTransport::PrepareAllToAllRound(const AllToAllPacketRequest& request,
    std::size_t packets_per_peer, PacketRoundLayout& layout) const noexcept
{
    layout = {};

    for (std::size_t index = 0; index < send_progress_.size(); ++index) {
        const std::size_t send_remaining =
            static_cast<std::size_t>(request.send_counts[index]) - send_progress_[index];

        const std::size_t receive_remaining =
            static_cast<std::size_t>(request.receive_counts[index]) - receive_progress_[index];

        const std::size_t send_chunk = std::min(send_remaining, packets_per_peer);
        const std::size_t receive_chunk = std::min(receive_remaining, packets_per_peer);

        if (!MpiPacketProtocol::ToWireBytes(layout.send_total, send_offsets_[index]) ||
            !MpiPacketProtocol::ToWireBytes(send_chunk, send_bytes_[index]) ||
            !MpiPacketProtocol::ToWireBytes(layout.receive_total, receive_offsets_[index]) ||
            !MpiPacketProtocol::ToWireBytes(receive_chunk, receive_bytes_[index]) ||
            layout.send_total > std::numeric_limits<std::size_t>::max() - send_chunk ||
            layout.receive_total > std::numeric_limits<std::size_t>::max() - receive_chunk) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        layout.send_total += send_chunk;
        layout.receive_total += receive_chunk;
    }

    if (!MpiPacketProtocol::ToWireBytes(layout.send_total, layout.send_total_bytes) ||
        !MpiPacketProtocol::ToWireBytes(layout.receive_total, layout.receive_total_bytes) ||
        !MpiPacketProtocol::ResizeWithinCapacity(
            send_wire_, static_cast<std::size_t>(layout.send_total_bytes)) ||
        !MpiPacketProtocol::ResizeWithinCapacity(
            receive_wire_, static_cast<std::size_t>(layout.receive_total_bytes))) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiPacketTransport::EncodeAllToAll(
    const AllToAllPacketRequest& request) const noexcept
{
    for (std::size_t index = 0; index < send_progress_.size(); ++index) {
        const std::size_t chunk = static_cast<std::size_t>(send_bytes_[index] / ParticleWireBytes);

        const std::size_t source_offset =
            static_cast<std::size_t>(request.send_displacements[index]) + send_progress_[index];

        if (!ParticleWireCodec::Encode(request.send_packets.subspan(source_offset, chunk),
                std::span<std::byte>(send_wire_.data(), send_wire_.size())
                    .subspan(static_cast<std::size_t>(send_offsets_[index]),
                        static_cast<std::size_t>(send_bytes_[index])))) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiPacketTransport::DecodeAllToAll(
    const AllToAllPacketRequest& request) const noexcept
{
    for (std::size_t index = 0; index < receive_progress_.size(); ++index) {
        const std::size_t chunk =
            static_cast<std::size_t>(receive_bytes_[index] / ParticleWireBytes);

        const std::size_t target_offset =
            static_cast<std::size_t>(request.receive_displacements[index]) +
            receive_progress_[index];

        if (!ParticleWireCodec::Decode(
                std::span<const std::byte>(receive_wire_.data(), receive_wire_.size())
                    .subspan(static_cast<std::size_t>(receive_offsets_[index]),
                        static_cast<std::size_t>(receive_bytes_[index])),
                request.receive_packets.subspan(target_offset, chunk))) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_parallel
