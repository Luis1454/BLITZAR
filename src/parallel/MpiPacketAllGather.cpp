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

blitzar_status MpiPacketTransport::AllGatherPackets(std::span<const ParticlePacket> local_packets,
    std::span<ParticlePacket> gathered_packets, std::span<const int> counts,
    std::span<const int> displacements) const noexcept
{
    const bool layout_valid =
        counts.size() == static_cast<std::size_t>(session_.Size()) &&
        displacements.size() == static_cast<std::size_t>(session_.Size()) &&
        MpiPacketProtocol::ValidateLayout(counts, displacements, gathered_packets.size());
    const bool local_count_valid =
        layout_valid &&
        local_packets.size() == static_cast<std::size_t>(counts[static_cast<std::size_t>(session_.Rank())]);

    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        if (!local_count_valid) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        std::copy_n(local_packets.begin(), local_packets.size(),
            gathered_packets.begin() + displacements[0]);

        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)

    blitzar_status preparation_status = layout_valid && local_count_valid
                                             ? BLITZAR_STATUS_OK
                                             : BLITZAR_STATUS_INVALID_ARGUMENT;

    std::size_t packets_per_peer = 0;

    if (preparation_status == BLITZAR_STATUS_OK &&
        !MpiPacketProtocol::ComputeRoundPacketLimit(session_.Size(), packets_per_peer)) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t peer_count = static_cast<std::size_t>(session_.Size());

    if (send_progress_.size() != peer_count || receive_bytes_.size() != peer_count ||
        receive_offsets_.size() != peer_count) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::vector<std::size_t>& progress = send_progress_;
    std::vector<int>& receive_bytes = receive_bytes_;
    std::vector<int>& receive_offsets = receive_offsets_;
    std::vector<std::byte>& send_wire = send_wire_;
    std::vector<std::byte>& receive_wire = receive_wire_;

    std::fill(progress.begin(), progress.end(), 0);
    std::fill(receive_bytes.begin(), receive_bytes.end(), 0);
    std::fill(receive_offsets.begin(), receive_offsets.end(), 0);

    blitzar_status status = MpiPacketProtocol::SynchronizePreparation(
        collectives_, preparation_status, "allgather-packet-prepare");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    for (;;) {
        const int local_more = MpiPacketProtocol::HasRemaining(counts, progress) ? 1 : 0;
        int global_more = 0;

        if (collectives_.ReduceMax(local_more, global_more) != BLITZAR_STATUS_OK) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        if (global_more == 0) {
            return BLITZAR_STATUS_OK;
        }

        std::size_t receive_total = 0;

        preparation_status = BLITZAR_STATUS_OK;

        for (std::size_t index = 0; index < peer_count; ++index) {
            const std::size_t remaining = static_cast<std::size_t>(counts[index]) - progress[index];
            const std::size_t chunk = std::min(remaining, packets_per_peer);

            if (!MpiPacketProtocol::ToWireBytes(receive_total, receive_offsets[index]) ||
                !MpiPacketProtocol::ToWireBytes(chunk, receive_bytes[index]) ||
                receive_total > std::numeric_limits<std::size_t>::max() - chunk) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;

                break;
            }

            receive_total += chunk;
        }

        int local_bytes = 0;
        int receive_total_bytes = 0;
        const std::size_t local_index = static_cast<std::size_t>(session_.Rank());
        const std::size_t local_remaining =
            static_cast<std::size_t>(counts[local_index]) - progress[local_index];
        const std::size_t local_chunk = std::min(local_remaining, packets_per_peer);

        if (preparation_status == BLITZAR_STATUS_OK &&
            (!MpiPacketProtocol::ToWireBytes(local_chunk, local_bytes) ||
                !MpiPacketProtocol::ToWireBytes(receive_total, receive_total_bytes))) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        if (preparation_status == BLITZAR_STATUS_OK) {
            const blitzar_status send_capacity_status = MpiPacketProtocol::EnsureCapacity(
                send_wire, static_cast<std::size_t>(local_bytes));
            const blitzar_status receive_capacity_status = MpiPacketProtocol::EnsureCapacity(
                receive_wire, static_cast<std::size_t>(receive_total_bytes));

            if (send_capacity_status != BLITZAR_STATUS_OK ||
                receive_capacity_status != BLITZAR_STATUS_OK) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;

                if (send_capacity_status == BLITZAR_STATUS_ALLOCATION_FAILURE ||
                    receive_capacity_status == BLITZAR_STATUS_ALLOCATION_FAILURE) {
                    preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
                }
            }
            else if (!MpiPacketProtocol::ResizeWithinCapacity(
                         send_wire, static_cast<std::size_t>(local_bytes)) ||
                     !MpiPacketProtocol::ResizeWithinCapacity(
                         receive_wire, static_cast<std::size_t>(receive_total_bytes))) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
            }
        }
        if (preparation_status == BLITZAR_STATUS_OK &&
            !ParticleWireCodec::Encode(local_packets.subspan(progress[local_index], local_chunk),
                std::span<std::byte>(send_wire.data(), send_wire.size()))) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        status = MpiPacketProtocol::SynchronizePreparation(
            collectives_, preparation_status, "allgather-packet-round-prepare");

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        if (MPI_Allgatherv(send_wire.data(), local_bytes, MPI_BYTE, receive_wire.data(),
                receive_bytes.data(), receive_offsets.data(), MPI_BYTE,
                session_.Native().communicator) != MPI_SUCCESS) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        blitzar_status decode_status = BLITZAR_STATUS_OK;

        for (std::size_t index = 0; index < peer_count; ++index) {
            const std::size_t chunk =
                static_cast<std::size_t>(receive_bytes[index] / ParticleWireBytes);

            if (!ParticleWireCodec::Decode(
                    std::span<const std::byte>(receive_wire.data(), receive_wire.size())
                        .subspan(static_cast<std::size_t>(receive_offsets[index]),
                            static_cast<std::size_t>(receive_bytes[index])),
                    gathered_packets.subspan(
                        static_cast<std::size_t>(displacements[index]) + progress[index], chunk))) {
                decode_status = BLITZAR_STATUS_INVALID_ARGUMENT;

                break;
            }
        }

        status = MpiPacketProtocol::SynchronizePreparation(
            collectives_, decode_status, "allgather-packet-round-decode");

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        for (std::size_t index = 0; index < peer_count; ++index) {
            progress[index] += static_cast<std::size_t>(receive_bytes[index] / ParticleWireBytes);
        }
    }
#else

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

} // namespace blitzar_parallel
