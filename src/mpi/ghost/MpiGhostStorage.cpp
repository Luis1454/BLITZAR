#include "mpi/ghost/MpiGhostProtocol.hpp"
#include "mpi/ghost/MpiGhostState.hpp"
#include "mpi/ghost/MpiGhostTransport.hpp"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <limits>
#include <memory>

namespace blitzar_parallel {

blitzar_status MpiGhostTransport::PrepareStorage(MpiGhostExchange::Impl& state,
    std::size_t send_capacity, std::size_t receive_capacity) const noexcept
{
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
    state.receive_byte_counts.reserve(request_capacity);
    state.offsets.resize(peer_count);
    state.receive_chunks.reserve(request_capacity);

    if (!MpiGhostProtocol::EnsureCapacity(state.local_wire, local_wire_capacity) ||
        !MpiGhostProtocol::EnsureCapacity(state.receive_wire, receive_wire_capacity)) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    if (state.native == nullptr) {
        state.native = std::make_unique<MpiNativeGhost>();
    }
    if (state.native == nullptr) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    const blitzar_status native_status =
        session_.Native().ReserveGhost(*state.native, request_capacity, request_capacity);

    if (native_status != BLITZAR_STATUS_OK) {
        return native_status;
    }

    state.local_wire.clear();
    state.receive_wire.clear();
    state.receive_byte_counts.clear();
    state.receive_chunks.clear();
    std::fill(state.receive_counts.begin(), state.receive_counts.end(), 0);
    std::fill(state.offsets.begin(), state.offsets.end(), 0);

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_parallel
