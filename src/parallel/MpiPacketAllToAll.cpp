#include "parallel/MpiPacketTransport.hpp"

#include "parallel/MpiPacketProtocol.hpp"
#include "parallel/MpiSessionNative.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <vector>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

blitzar_status MpiPacketTransport::AllToAllPackets(
    const AllToAllPacketRequest& request) const noexcept
{
    const std::span<const ParticlePacket> send_packets = request.send_packets;
    const std::span<const int> send_counts = request.send_counts;
    const std::span<const int> send_displacements = request.send_displacements;
    const std::span<ParticlePacket> receive_packets = request.receive_packets;
    const std::span<const int> receive_counts = request.receive_counts;
    const std::span<const int> receive_displacements = request.receive_displacements;

    const bool layout_valid =
        send_counts.size() == static_cast<std::size_t>(session_.Size()) &&
        send_displacements.size() == static_cast<std::size_t>(session_.Size()) &&
        receive_counts.size() == static_cast<std::size_t>(session_.Size()) &&
        receive_displacements.size() == static_cast<std::size_t>(session_.Size()) &&
        MpiPacketProtocol::ValidateLayout(send_counts, send_displacements, send_packets.size()) &&
        MpiPacketProtocol::ValidateLayout(
            receive_counts, receive_displacements, receive_packets.size());

    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        if (!layout_valid || send_counts[0] != receive_counts[0]) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        std::copy_n(send_packets.begin() + send_displacements[0],
            static_cast<std::size_t>(send_counts[0]),
            receive_packets.begin() + receive_displacements[0]);

        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)

    blitzar_status preparation_status =
        layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT;

    std::size_t packets_per_peer = 0;

    if (preparation_status == BLITZAR_STATUS_OK &&
        !MpiPacketProtocol::ComputeRoundPacketLimit(session_.Size(), packets_per_peer)) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t peer_count = static_cast<std::size_t>(session_.Size());

    if (send_progress_.size() != peer_count || receive_progress_.size() != peer_count ||
        send_bytes_.size() != peer_count || receive_bytes_.size() != peer_count ||
        send_offsets_.size() != peer_count || receive_offsets_.size() != peer_count) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::vector<std::size_t>& send_progress = send_progress_;
    std::vector<std::size_t>& receive_progress = receive_progress_;
    std::vector<int>& send_bytes = send_bytes_;
    std::vector<int>& receive_bytes = receive_bytes_;
    std::vector<int>& send_offsets = send_offsets_;
    std::vector<int>& receive_offsets = receive_offsets_;
    std::vector<std::byte>& send_wire = send_wire_;
    std::vector<std::byte>& receive_wire = receive_wire_;

    std::fill(send_progress.begin(), send_progress.end(), 0);
    std::fill(receive_progress.begin(), receive_progress.end(), 0);
    std::fill(send_bytes.begin(), send_bytes.end(), 0);
    std::fill(receive_bytes.begin(), receive_bytes.end(), 0);
    std::fill(send_offsets.begin(), send_offsets.end(), 0);
    std::fill(receive_offsets.begin(), receive_offsets.end(), 0);

    blitzar_status status = MpiPacketProtocol::SynchronizePreparation(
        collectives_, preparation_status, "alltoall-packet-prepare");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    for (;;) {
        const int local_more = MpiPacketProtocol::HasRemaining(send_counts, send_progress) ||
                                       MpiPacketProtocol::HasRemaining(
                                           receive_counts, receive_progress)
                                   ? 1
                                   : 0;
        int global_more = 0;

        if (collectives_.ReduceMax(local_more, global_more) != BLITZAR_STATUS_OK) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        if (global_more == 0) {
            return BLITZAR_STATUS_OK;
        }

        std::size_t send_total = 0;
        std::size_t receive_total = 0;

        preparation_status = BLITZAR_STATUS_OK;

        for (std::size_t index = 0; index < peer_count; ++index) {
            const std::size_t send_remaining =
                static_cast<std::size_t>(send_counts[index]) - send_progress[index];
            const std::size_t receive_remaining =
                static_cast<std::size_t>(receive_counts[index]) - receive_progress[index];
            const std::size_t send_chunk = std::min(send_remaining, packets_per_peer);
            const std::size_t receive_chunk = std::min(receive_remaining, packets_per_peer);

            if (!MpiPacketProtocol::ToWireBytes(send_total, send_offsets[index]) ||
                !MpiPacketProtocol::ToWireBytes(send_chunk, send_bytes[index]) ||
                !MpiPacketProtocol::ToWireBytes(receive_total, receive_offsets[index]) ||
                !MpiPacketProtocol::ToWireBytes(receive_chunk, receive_bytes[index]) ||
                send_total > std::numeric_limits<std::size_t>::max() - send_chunk ||
                receive_total > std::numeric_limits<std::size_t>::max() - receive_chunk) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;

                break;
            }

            send_total += send_chunk;
            receive_total += receive_chunk;
        }

        int send_total_bytes = 0;
        int receive_total_bytes = 0;

        if (preparation_status == BLITZAR_STATUS_OK &&
            (!MpiPacketProtocol::ToWireBytes(send_total, send_total_bytes) ||
                !MpiPacketProtocol::ToWireBytes(receive_total, receive_total_bytes))) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        if (preparation_status == BLITZAR_STATUS_OK) {
            const blitzar_status send_capacity_status = MpiPacketProtocol::EnsureCapacity(
                send_wire, static_cast<std::size_t>(send_total_bytes));
            const blitzar_status receive_capacity_status = MpiPacketProtocol::EnsureCapacity(
                receive_wire, static_cast<std::size_t>(receive_total_bytes));

            if (send_capacity_status != BLITZAR_STATUS_OK ||
                receive_capacity_status != BLITZAR_STATUS_OK ||
                !MpiPacketProtocol::ResizeWithinCapacity(
                    send_wire, static_cast<std::size_t>(send_total_bytes)) ||
                !MpiPacketProtocol::ResizeWithinCapacity(
                    receive_wire, static_cast<std::size_t>(receive_total_bytes))) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;

                if (send_capacity_status == BLITZAR_STATUS_ALLOCATION_FAILURE ||
                    receive_capacity_status == BLITZAR_STATUS_ALLOCATION_FAILURE) {
                    preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
                }
            }
        }
        if (preparation_status == BLITZAR_STATUS_OK) {
            for (std::size_t index = 0; index < peer_count; ++index) {
                const std::size_t chunk =
                    static_cast<std::size_t>(send_bytes[index] / ParticleWireBytes);
                const std::size_t source_offset =
                    static_cast<std::size_t>(send_displacements[index]) + send_progress[index];

                if (!ParticleWireCodec::Encode(send_packets.subspan(source_offset, chunk),
                        std::span<std::byte>(send_wire.data(), send_wire.size())
                            .subspan(static_cast<std::size_t>(send_offsets[index]),
                                static_cast<std::size_t>(send_bytes[index])))) {
                    preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;

                    break;
                }
            }
        }

        status = MpiPacketProtocol::SynchronizePreparation(
            collectives_, preparation_status, "alltoall-packet-round-prepare");

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        if (MPI_Alltoallv(send_wire.data(), send_bytes.data(), send_offsets.data(), MPI_BYTE,
                receive_wire.data(), receive_bytes.data(), receive_offsets.data(), MPI_BYTE,
                session_.Native().communicator) != MPI_SUCCESS) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        blitzar_status decode_status = BLITZAR_STATUS_OK;

        for (std::size_t index = 0; index < peer_count; ++index) {
            const std::size_t chunk =
                static_cast<std::size_t>(receive_bytes[index] / ParticleWireBytes);
            const std::size_t target_offset =
                static_cast<std::size_t>(receive_displacements[index]) + receive_progress[index];

            if (!ParticleWireCodec::Decode(
                    std::span<const std::byte>(receive_wire.data(), receive_wire.size())
                        .subspan(static_cast<std::size_t>(receive_offsets[index]),
                            static_cast<std::size_t>(receive_bytes[index])),
                    receive_packets.subspan(target_offset, chunk))) {
                decode_status = BLITZAR_STATUS_INVALID_ARGUMENT;

                break;
            }
        }

        status = MpiPacketProtocol::SynchronizePreparation(
            collectives_, decode_status, "alltoall-packet-round-decode");

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        for (std::size_t index = 0; index < peer_count; ++index) {
            send_progress[index] += static_cast<std::size_t>(send_bytes[index] / ParticleWireBytes);
            receive_progress[index] +=
                static_cast<std::size_t>(receive_bytes[index] / ParticleWireBytes);
        }
    }
#else

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

} // namespace blitzar_parallel
