#include "parallel/MpiGhostProtocol.hpp"
#include "parallel/MpiGhostState.hpp"
#include "parallel/MpiGhostTransport.hpp"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <limits>

namespace blitzar_parallel {

blitzar_status MpiGhostTransport::PrepareStorage(MpiGhostExchange::Impl& state,
    std::size_t send_capacity, std::size_t receive_capacity) const noexcept
{
#if defined(BLITZAR_HAS_MPI)
    const std::size_t peer_count = static_cast<std::size_t>(session_.Size());
    const std::size_t remote_peer_count = peer_count - 1;
    const std::size_t chunks = std::max(MpiGhostProtocol::ChunkCount(send_capacity),
        MpiGhostProtocol::ChunkCount(receive_capacity));

    if (remote_peer_count != 0 &&
        chunks > std::numeric_limits<std::size_t>::max() / remote_peer_count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t request_capacity = chunks * remote_peer_count;
    std::size_t local_wire_capacity = 0;
    std::size_t receive_wire_capacity = 0;

    if (request_capacity > static_cast<std::size_t>(INT_MAX) ||
        !MpiGhostProtocol::ToWireSize(send_capacity, local_wire_capacity) ||
        !MpiGhostProtocol::ToWireSize(receive_capacity, receive_wire_capacity)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    state.send_capacity = send_capacity;
    state.receive_capacity = receive_capacity;

    state.peer_counts.resize(peer_count);
    state.peer_capacities.resize(peer_count);
    state.wire_offsets.resize(peer_count);
    state.receive_counts.resize(peer_count);
    state.offsets.resize(peer_count);
    state.receive_requests.reserve(request_capacity);
    state.send_requests.reserve(request_capacity);
    state.receive_statuses.reserve(request_capacity);
    state.receive_chunks.reserve(request_capacity);

    if (!MpiGhostProtocol::EnsureCapacity(state.local_wire, local_wire_capacity) ||
        !MpiGhostProtocol::EnsureCapacity(state.receive_wire, receive_wire_capacity)) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    state.local_wire.clear();
    state.receive_wire.clear();
    state.receive_requests.clear();
    state.send_requests.clear();
    state.receive_statuses.clear();
    state.receive_chunks.clear();
    std::fill(state.receive_counts.begin(), state.receive_counts.end(), 0);
    std::fill(state.offsets.begin(), state.offsets.end(), 0);

    return BLITZAR_STATUS_OK;
#else

    (void)state;
    (void)send_capacity;
    (void)receive_capacity;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

} // namespace blitzar_parallel
