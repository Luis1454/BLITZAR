#include "parallel/MpiPacketTransport.hpp"

#include "parallel/MpiPacketProtocol.hpp"
#include "parallel/MpiSessionNative.hpp"

#include <cstddef>
#include <limits>
#include <new>
#include <stdexcept>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

MpiPacketTransport::MpiPacketTransport(
    const MpiSession& session, const MpiCollectives& collectives) noexcept
    : session_(session), collectives_(collectives)
{
}

blitzar_status MpiPacketTransport::Prepare(std::size_t packet_capacity) noexcept
{
    const std::size_t peer_count =
        session_.Size() > 0 ? static_cast<std::size_t>(session_.Size()) : 0;

    try {
        send_progress_.assign(peer_count, 0);
        receive_progress_.assign(peer_count, 0);
        send_bytes_.assign(peer_count, 0);
        receive_bytes_.assign(peer_count, 0);
        send_offsets_.assign(peer_count, 0);
        receive_offsets_.assign(peer_count, 0);

#if defined(BLITZAR_HAS_MPI)

        std::size_t packets_per_peer = 0;

        if (!MpiPacketProtocol::ComputeRoundPacketLimit(session_.Size(), packets_per_peer)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const std::size_t peers = peer_count;

        if (peers != 0 && packets_per_peer > std::numeric_limits<std::size_t>::max() / peers) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const std::size_t round_capacity = std::min(packet_capacity, packets_per_peer * peers);
        int send_wire_bytes = 0;
        int receive_wire_bytes = 0;

        if (!MpiPacketProtocol::ToWireBytes(round_capacity, send_wire_bytes) ||
            !MpiPacketProtocol::ToWireBytes(packet_capacity, receive_wire_bytes)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        send_wire_.reserve(static_cast<std::size_t>(send_wire_bytes));
        receive_wire_.reserve(static_cast<std::size_t>(receive_wire_bytes));
#else

        (void)packet_capacity;
#endif
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiPacketTransport::AllToAllCounts(
    std::span<const int> send_counts, std::span<int> receive_counts) const noexcept
{
    bool layout_valid = send_counts.size() == static_cast<std::size_t>(session_.Size()) &&
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
        layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT, "MpiPacketTransport",
        "alltoall-count-layout", global_layout_status);

    if (synchronization_status != BLITZAR_STATUS_OK || global_layout_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status
                                                           : global_layout_status;
    }

    return MPI_Alltoall(send_counts.data(), 1, MPI_INT, receive_counts.data(), 1, MPI_INT,
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
        local_count >= 0 && counts.size() == static_cast<std::size_t>(session_.Size());

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
        layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT, "MpiPacketTransport",
        "allgather-count-layout", global_layout_status);

    if (synchronization_status != BLITZAR_STATUS_OK || global_layout_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status
                                                           : global_layout_status;
    }

    return MPI_Allgather(&local_count, 1, MPI_INT, counts.data(), 1, MPI_INT,
               session_.Native().communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
#else

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

} // namespace blitzar_parallel
