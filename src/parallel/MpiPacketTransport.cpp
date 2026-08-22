#include "parallel/MpiPacketTransport.hpp"

#include "parallel/MpiSessionNative.hpp"

#include <algorithm>
#include <climits>
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

[[nodiscard]] bool ToByteCount(std::size_t packets, int& bytes) noexcept
{
    if (packets > static_cast<std::size_t>(INT_MAX) /
                          sizeof(ParticlePacket)) {
        return false;
    }
    bytes = static_cast<int>(packets * sizeof(ParticlePacket));
    return true;
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
            send_counts[0],
            receive_packets.begin() + receive_displacements[0]);
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    blitzar_status preparation_status = layout_valid
                                            ? BLITZAR_STATUS_OK
                                            : BLITZAR_STATUS_INVALID_ARGUMENT;
    blitzar_status global_preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
    blitzar_status synchronization_status = collectives_.SynchronizeStatus(
        preparation_status,
        "MpiPacketTransport",
        "alltoall-packet-layout",
        global_preparation_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_preparation_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_preparation_status;
    }
    std::vector<int> send_bytes;
    std::vector<int> receive_bytes;
    std::vector<int> send_offsets;
    std::vector<int> receive_offsets;
    try {
        send_bytes.assign(static_cast<std::size_t>(session_.Size()), 0);
        receive_bytes.assign(static_cast<std::size_t>(session_.Size()), 0);
        send_offsets.assign(static_cast<std::size_t>(session_.Size()), 0);
        receive_offsets.assign(static_cast<std::size_t>(session_.Size()), 0);
        for (int peer = 0; peer < session_.Size(); ++peer) {
            const std::size_t index = static_cast<std::size_t>(peer);
            if (!ToByteCount(
                    static_cast<std::size_t>(send_counts[index]),
                    send_bytes[index]) ||
                !ToByteCount(
                    static_cast<std::size_t>(receive_counts[index]),
                    receive_bytes[index]) ||
                !ToByteCount(
                    static_cast<std::size_t>(send_displacements[index]),
                    send_offsets[index]) ||
                !ToByteCount(
                    static_cast<std::size_t>(receive_displacements[index]),
                    receive_offsets[index])) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                break;
            }
        }
    } catch (const std::length_error&) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    synchronization_status = collectives_.SynchronizeStatus(
        preparation_status,
        "MpiPacketTransport",
        "alltoall-packet-prepare",
        global_preparation_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_preparation_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_preparation_status;
    }
    return MPI_Alltoallv(
                   send_packets.data(),
                   send_bytes.data(),
                   send_offsets.data(),
                   MPI_BYTE,
                   receive_packets.data(),
                   receive_bytes.data(),
                   receive_offsets.data(),
                   MPI_BYTE,
                   session_.Native().communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
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
        local_packets.size() ==
            static_cast<std::size_t>(
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
    blitzar_status global_preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
    blitzar_status synchronization_status = collectives_.SynchronizeStatus(
        preparation_status,
        "MpiPacketTransport",
        "allgather-packet-layout",
        global_preparation_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_preparation_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_preparation_status;
    }
    std::vector<int> bytes;
    std::vector<int> offsets;
    try {
        bytes.assign(static_cast<std::size_t>(session_.Size()), 0);
        offsets.assign(static_cast<std::size_t>(session_.Size()), 0);
        for (int peer = 0; peer < session_.Size(); ++peer) {
            const std::size_t index = static_cast<std::size_t>(peer);
            if (!ToByteCount(
                    static_cast<std::size_t>(counts[index]), bytes[index]) ||
                !ToByteCount(
                    static_cast<std::size_t>(displacements[index]),
                    offsets[index])) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                break;
            }
        }
        int local_bytes = 0;
        if (preparation_status == BLITZAR_STATUS_OK &&
            !ToByteCount(local_packets.size(), local_bytes)) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        synchronization_status = collectives_.SynchronizeStatus(
            preparation_status,
            "MpiPacketTransport",
            "allgather-packet-prepare",
            global_preparation_status);
        if (synchronization_status != BLITZAR_STATUS_OK ||
            global_preparation_status != BLITZAR_STATUS_OK) {
            return synchronization_status != BLITZAR_STATUS_OK
                       ? synchronization_status
                       : global_preparation_status;
        }
        return MPI_Allgatherv(
                   local_packets.data(),
                   local_bytes,
                   MPI_BYTE,
                   gathered_packets.data(),
                   bytes.data(),
                   offsets.data(),
                   MPI_BYTE,
                   session_.Native().communicator) == MPI_SUCCESS
                   ? BLITZAR_STATUS_OK
                   : BLITZAR_STATUS_INTERNAL_ERROR;
    } catch (const std::length_error&) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    synchronization_status = collectives_.SynchronizeStatus(
        preparation_status,
        "MpiPacketTransport",
        "allgather-packet-prepare",
        global_preparation_status);
    return synchronization_status != BLITZAR_STATUS_OK
               ? synchronization_status
               : global_preparation_status;
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

}  // namespace blitzar_parallel
