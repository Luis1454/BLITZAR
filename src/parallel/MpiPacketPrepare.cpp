#include "parallel/MpiPacketProtocol.hpp"
#include "parallel/MpiPacketTransport.hpp"
#include "parallel/MpiSessionNative.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <new>
#include <stdexcept>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

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

} // namespace blitzar_parallel
