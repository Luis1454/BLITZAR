#include "parallel/MpiPacketTransport.hpp"

#include "parallel/MpiSessionNative.hpp"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <limits>
#include <new>
#include <stdexcept>
#include <vector>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

namespace {

[[nodiscard]] bool ValidateLayout(
    std::span<const int> counts,
    std::span<const int> displacements,
    std::size_t packet_count) noexcept
{
    if (counts.size() != displacements.size()) {
        return false;
    }
    for (std::size_t index = 0; index < counts.size(); ++index) {
        if (counts[index] < 0 || displacements[index] < 0 ||
            static_cast<std::size_t>(displacements[index]) > packet_count ||
            static_cast<std::size_t>(counts[index]) >
                packet_count - static_cast<std::size_t>(displacements[index])) {
            return false;
        }
    }
    return true;
}

#if defined(BLITZAR_HAS_MPI)

[[nodiscard]] blitzar_status SynchronizePreparation(
    const MpiCollectives& collectives,
    blitzar_status local_status,
    const char* phase) noexcept
{
    blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        collectives.SynchronizeStatus(
            local_status,
            "MpiPacketTransport",
            phase,
            global_status);
    return synchronization_status != BLITZAR_STATUS_OK
               ? synchronization_status
               : global_status;
}

[[nodiscard]] bool ToWireBytes(std::size_t packets, int& bytes) noexcept
{
    if (packets > static_cast<std::size_t>(INT_MAX) / ParticleWireBytes) {
        return false;
    }
    bytes = static_cast<int>(packets * ParticleWireBytes);
    return true;
}

[[nodiscard]] bool ComputeRoundPacketLimit(
    int peer_count,
    std::size_t& packets_per_peer) noexcept
{
    if (peer_count <= 0 || ParticleWireBytes == 0) {
        return false;
    }
    const std::size_t peers = static_cast<std::size_t>(peer_count);
    if (peers > std::numeric_limits<std::size_t>::max() / ParticleWireBytes) {
        return false;
    }
    const std::size_t bytes_per_round_packet = peers * ParticleWireBytes;
    packets_per_peer =
        static_cast<std::size_t>(INT_MAX) / bytes_per_round_packet;
    return packets_per_peer != 0;
}

[[nodiscard]] bool HasRemaining(
    std::span<const int> counts,
    std::span<const std::size_t> progress) noexcept
{
    for (std::size_t index = 0; index < counts.size(); ++index) {
        if (progress[index] < static_cast<std::size_t>(counts[index])) {
            return true;
        }
    }
    return false;
}

#endif

}  // namespace

MpiPacketTransport::MpiPacketTransport(
    const MpiSession& session,
    const MpiCollectives& collectives) noexcept
    : session_(session), collectives_(collectives)
{
}

blitzar_status MpiPacketTransport::AllToAllCounts(
    std::span<const int> send_counts,
    std::span<int> receive_counts) const noexcept
{
    bool layout_valid =
        send_counts.size() == static_cast<std::size_t>(session_.Size()) &&
        receive_counts.size() == static_cast<std::size_t>(session_.Size());
    if (layout_valid) {
        for (const int count : send_counts) {
            if (count < 0) {
                layout_valid = false;
                break;
            }
        }
    }
    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        if (!layout_valid) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        receive_counts[0] = send_counts[0];
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    blitzar_status global_layout_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = collectives_.SynchronizeStatus(
        layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "MpiPacketTransport",
        "alltoall-count-layout",
        global_layout_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_layout_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_layout_status;
    }
    return MPI_Alltoall(
               send_counts.data(),
               1,
               MPI_INT,
               receive_counts.data(),
               1,
               MPI_INT,
               session_.Native().communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiPacketTransport::AllToAllPackets(
    std::span<const ParticlePacket> send_packets,
    std::span<const int> send_counts,
    std::span<const int> send_displacements,
    std::span<ParticlePacket> receive_packets,
    std::span<const int> receive_counts,
    std::span<const int> receive_displacements) const noexcept
{
    const bool layout_valid =
        send_counts.size() == static_cast<std::size_t>(session_.Size()) &&
        send_displacements.size() == static_cast<std::size_t>(session_.Size()) &&
        receive_counts.size() == static_cast<std::size_t>(session_.Size()) &&
        receive_displacements.size() == static_cast<std::size_t>(session_.Size()) &&
        ValidateLayout(send_counts, send_displacements, send_packets.size()) &&
        ValidateLayout(
            receive_counts, receive_displacements, receive_packets.size());
    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        if (!layout_valid || send_counts[0] != receive_counts[0]) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        std::copy_n(
            send_packets.begin() + send_displacements[0],
            static_cast<std::size_t>(send_counts[0]),
            receive_packets.begin() + receive_displacements[0]);
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    blitzar_status preparation_status = layout_valid
                                            ? BLITZAR_STATUS_OK
                                            : BLITZAR_STATUS_INVALID_ARGUMENT;
    std::size_t packets_per_peer = 0;
    if (preparation_status == BLITZAR_STATUS_OK &&
        !ComputeRoundPacketLimit(session_.Size(), packets_per_peer)) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t peer_count = static_cast<std::size_t>(session_.Size());
    std::vector<std::size_t> send_progress;
    std::vector<std::size_t> receive_progress;
    std::vector<int> send_bytes;
    std::vector<int> receive_bytes;
    std::vector<int> send_offsets;
    std::vector<int> receive_offsets;
    std::vector<std::byte> send_wire;
    std::vector<std::byte> receive_wire;
    try {
        send_progress.assign(peer_count, 0);
        receive_progress.assign(peer_count, 0);
        send_bytes.assign(peer_count, 0);
        receive_bytes.assign(peer_count, 0);
        send_offsets.assign(peer_count, 0);
        receive_offsets.assign(peer_count, 0);
    } catch (const std::length_error&) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    blitzar_status status = SynchronizePreparation(
        collectives_, preparation_status, "alltoall-packet-prepare");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    for (;;) {
        int local_more =
            HasRemaining(send_counts, send_progress) ||
                    HasRemaining(receive_counts, receive_progress)
                ? 1
                : 0;
        int global_more = 0;
        if (collectives_.ReduceMax(local_more, global_more) !=
            BLITZAR_STATUS_OK) {
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
                static_cast<std::size_t>(send_counts[index]) -
                send_progress[index];
            const std::size_t receive_remaining =
                static_cast<std::size_t>(receive_counts[index]) -
                receive_progress[index];
            const std::size_t send_chunk =
                std::min(send_remaining, packets_per_peer);
            const std::size_t receive_chunk =
                std::min(receive_remaining, packets_per_peer);
            if (!ToWireBytes(send_total, send_offsets[index]) ||
                !ToWireBytes(send_chunk, send_bytes[index]) ||
                !ToWireBytes(receive_total, receive_offsets[index]) ||
                !ToWireBytes(receive_chunk, receive_bytes[index]) ||
                send_total > std::numeric_limits<std::size_t>::max() -
                                  send_chunk ||
                receive_total > std::numeric_limits<std::size_t>::max() -
                                     receive_chunk) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                break;
            }
            send_total += send_chunk;
            receive_total += receive_chunk;
        }

        int send_total_bytes = 0;
        int receive_total_bytes = 0;
        if (preparation_status == BLITZAR_STATUS_OK &&
            (!ToWireBytes(send_total, send_total_bytes) ||
             !ToWireBytes(receive_total, receive_total_bytes))) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        try {
            if (preparation_status == BLITZAR_STATUS_OK) {
                send_wire.resize(static_cast<std::size_t>(send_total_bytes));
                receive_wire.resize(
                    static_cast<std::size_t>(receive_total_bytes));
                for (std::size_t index = 0; index < peer_count; ++index) {
                    const std::size_t chunk = static_cast<std::size_t>(
                        send_bytes[index] / ParticleWireBytes);
                    const std::size_t source_offset =
                        static_cast<std::size_t>(send_displacements[index]) +
                        send_progress[index];
                    if (!ParticleWireCodec::Encode(
                            send_packets.subspan(source_offset, chunk),
                            std::span<std::byte>(
                                send_wire.data(), send_wire.size())
                                .subspan(
                                    static_cast<std::size_t>(send_offsets[index]),
                                    static_cast<std::size_t>(send_bytes[index])))) {
                        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                        break;
                    }
                }
            }
        } catch (const std::length_error&) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        } catch (const std::bad_alloc&) {
            preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
        }
        status = SynchronizePreparation(
            collectives_, preparation_status, "alltoall-packet-round-prepare");
        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        if (MPI_Alltoallv(
                send_wire.data(),
                send_bytes.data(),
                send_offsets.data(),
                MPI_BYTE,
                receive_wire.data(),
                receive_bytes.data(),
                receive_offsets.data(),
                MPI_BYTE,
                session_.Native().communicator) != MPI_SUCCESS) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        blitzar_status decode_status = BLITZAR_STATUS_OK;
        for (std::size_t index = 0; index < peer_count; ++index) {
            const std::size_t chunk = static_cast<std::size_t>(
                receive_bytes[index] / ParticleWireBytes);
            const std::size_t target_offset =
                static_cast<std::size_t>(receive_displacements[index]) +
                receive_progress[index];
            if (!ParticleWireCodec::Decode(
                    std::span<const std::byte>(
                        receive_wire.data(), receive_wire.size())
                        .subspan(
                            static_cast<std::size_t>(receive_offsets[index]),
                            static_cast<std::size_t>(receive_bytes[index])),
                    receive_packets.subspan(target_offset, chunk))) {
                decode_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                break;
            }
        }
        status = SynchronizePreparation(
            collectives_, decode_status, "alltoall-packet-round-decode");
        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
        for (std::size_t index = 0; index < peer_count; ++index) {
            send_progress[index] += static_cast<std::size_t>(
                send_bytes[index] / ParticleWireBytes);
            receive_progress[index] += static_cast<std::size_t>(
                receive_bytes[index] / ParticleWireBytes);
        }
    }
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiPacketTransport::AllGatherCounts(
    int local_count, std::span<int> counts) const noexcept
{
    const bool layout_valid =
        local_count >= 0 &&
        counts.size() == static_cast<std::size_t>(session_.Size());
    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        if (!layout_valid) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        counts[0] = local_count;
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    blitzar_status global_layout_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = collectives_.SynchronizeStatus(
        layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "MpiPacketTransport",
        "allgather-count-layout",
        global_layout_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_layout_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_layout_status;
    }
    return MPI_Allgather(
               &local_count,
               1,
               MPI_INT,
               counts.data(),
               1,
               MPI_INT,
               session_.Native().communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiPacketTransport::AllGatherPackets(
    std::span<const ParticlePacket> local_packets,
    std::span<ParticlePacket> gathered_packets,
    std::span<const int> counts,
    std::span<const int> displacements) const noexcept
{
    const bool layout_valid =
        counts.size() == static_cast<std::size_t>(session_.Size()) &&
        displacements.size() == static_cast<std::size_t>(session_.Size()) &&
        ValidateLayout(counts, displacements, gathered_packets.size());
    const bool local_count_valid =
        layout_valid &&
        local_packets.size() == static_cast<std::size_t>(
                                    counts[static_cast<std::size_t>(session_.Rank())]);
    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        if (!local_count_valid) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        std::copy_n(
            local_packets.begin(),
            local_packets.size(),
            gathered_packets.begin() + displacements[0]);
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    blitzar_status preparation_status =
        layout_valid && local_count_valid ? BLITZAR_STATUS_OK
                                          : BLITZAR_STATUS_INVALID_ARGUMENT;
    std::size_t packets_per_peer = 0;
    if (preparation_status == BLITZAR_STATUS_OK &&
        !ComputeRoundPacketLimit(session_.Size(), packets_per_peer)) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t peer_count = static_cast<std::size_t>(session_.Size());
    std::vector<std::size_t> progress;
    std::vector<int> receive_bytes;
    std::vector<int> receive_offsets;
    std::vector<std::byte> send_wire;
    std::vector<std::byte> receive_wire;
    try {
        progress.assign(peer_count, 0);
        receive_bytes.assign(peer_count, 0);
        receive_offsets.assign(peer_count, 0);
    } catch (const std::length_error&) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    blitzar_status status = SynchronizePreparation(
        collectives_, preparation_status, "allgather-packet-prepare");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    for (;;) {
        int local_more = HasRemaining(counts, progress) ? 1 : 0;
        int global_more = 0;
        if (collectives_.ReduceMax(local_more, global_more) !=
            BLITZAR_STATUS_OK) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        if (global_more == 0) {
            return BLITZAR_STATUS_OK;
        }

        std::size_t receive_total = 0;
        preparation_status = BLITZAR_STATUS_OK;
        for (std::size_t index = 0; index < peer_count; ++index) {
            const std::size_t remaining =
                static_cast<std::size_t>(counts[index]) - progress[index];
            const std::size_t chunk = std::min(remaining, packets_per_peer);
            if (!ToWireBytes(receive_total, receive_offsets[index]) ||
                !ToWireBytes(chunk, receive_bytes[index]) ||
                receive_total > std::numeric_limits<std::size_t>::max() -
                                    chunk) {
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
        const std::size_t local_chunk =
            std::min(local_remaining, packets_per_peer);
        if (preparation_status == BLITZAR_STATUS_OK &&
            (!ToWireBytes(local_chunk, local_bytes) ||
             !ToWireBytes(receive_total, receive_total_bytes))) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        try {
            if (preparation_status == BLITZAR_STATUS_OK) {
                send_wire.resize(static_cast<std::size_t>(local_bytes));
                receive_wire.resize(
                    static_cast<std::size_t>(receive_total_bytes));
                if (!ParticleWireCodec::Encode(
                        local_packets.subspan(progress[local_index], local_chunk),
                        std::span<std::byte>(
                            send_wire.data(), send_wire.size()))) {
                    preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                }
            }
        } catch (const std::length_error&) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        } catch (const std::bad_alloc&) {
            preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
        }
        status = SynchronizePreparation(
            collectives_,
            preparation_status,
            "allgather-packet-round-prepare");
        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        if (MPI_Allgatherv(
                send_wire.data(),
                local_bytes,
                MPI_BYTE,
                receive_wire.data(),
                receive_bytes.data(),
                receive_offsets.data(),
                MPI_BYTE,
                session_.Native().communicator) != MPI_SUCCESS) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        blitzar_status decode_status = BLITZAR_STATUS_OK;
        for (std::size_t index = 0; index < peer_count; ++index) {
            const std::size_t chunk = static_cast<std::size_t>(
                receive_bytes[index] / ParticleWireBytes);
            if (!ParticleWireCodec::Decode(
                    std::span<const std::byte>(
                        receive_wire.data(), receive_wire.size())
                        .subspan(
                            static_cast<std::size_t>(receive_offsets[index]),
                            static_cast<std::size_t>(receive_bytes[index])),
                    gathered_packets.subspan(
                        static_cast<std::size_t>(displacements[index]) +
                            progress[index],
                        chunk))) {
                decode_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                break;
            }
        }
        status = SynchronizePreparation(
            collectives_, decode_status, "allgather-packet-round-decode");
        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
        for (std::size_t index = 0; index < peer_count; ++index) {
            progress[index] += static_cast<std::size_t>(
                receive_bytes[index] / ParticleWireBytes);
        }
    }
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

}  // namespace blitzar_parallel
