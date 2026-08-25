#include "parallel/MpiGhostTransport.hpp"

#include "parallel/MpiGhostProtocol.hpp"
#include "parallel/MpiGhostState.hpp"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <limits>

namespace blitzar_parallel {

blitzar_status MpiGhostTransport::PrepareLayout(std::size_t local_size, std::size_t local_bytes,
    MpiGhostExchange::Impl& state, BeginLayout& layout) const noexcept
{
    const blitzar_status layout_status = PreparePeerLayout(local_size, local_bytes, state, layout);

    return layout_status == BLITZAR_STATUS_OK ? PrepareRoundStorage(state, layout) : layout_status;
}

blitzar_status MpiGhostTransport::PreparePeerLayout(std::size_t local_size,
    std::size_t local_bytes, MpiGhostExchange::Impl& state, BeginLayout& layout) const noexcept
{
    const std::size_t peer_count = static_cast<std::size_t>(session_.Size());
    const std::size_t remote_peer_count = peer_count - 1;

    layout.chunk_packets = MpiGhostProtocol::PointChunkPackets();

    if (layout.chunk_packets == 0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::fill(state.peer_capacities.begin(), state.peer_capacities.end(), 0);
    std::fill(state.wire_offsets.begin(), state.wire_offsets.end(), 0);

    for (std::size_t peer = 0; peer < peer_count; ++peer) {
        if (!PreparePeerCapacity(peer, state, layout)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    const blitzar_status local_status =
        PrepareLocalLayout(local_size, remote_peer_count, layout);

    return local_status == BLITZAR_STATUS_OK
               ? PrepareWireLayout(local_bytes, remote_peer_count, layout)
               : local_status;
}

bool MpiGhostTransport::PreparePeerCapacity(
    std::size_t peer, MpiGhostExchange::Impl& state, BeginLayout& layout) const noexcept
{
    const int peer_count = state.peer_counts[peer];

    if (peer_count < 0) {
        return false;
    }

    state.peer_capacities[peer] = static_cast<std::size_t>(peer_count);

    if (peer == static_cast<std::size_t>(session_.Rank())) {
        return true;
    }

    state.wire_offsets[peer] = layout.receive_slots;

    const std::size_t peer_chunks = MpiGhostProtocol::ChunkCount(state.peer_capacities[peer]);

    if (layout.receive_slots > std::numeric_limits<std::size_t>::max() -
                                    state.peer_capacities[peer] ||
        layout.receive_request_count >
            std::numeric_limits<std::size_t>::max() - peer_chunks) {
        return false;
    }

    layout.receive_slots += state.peer_capacities[peer];
    layout.receive_request_count += peer_chunks;

    return true;
}

blitzar_status MpiGhostTransport::PrepareLocalLayout(
    std::size_t local_size, std::size_t remote_peer_count, BeginLayout& layout) const noexcept
{
    const std::size_t local_chunks = MpiGhostProtocol::ChunkCount(local_size);

    if (remote_peer_count != 0 &&
        local_chunks > std::numeric_limits<std::size_t>::max() / remote_peer_count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    layout.send_request_count = local_chunks * remote_peer_count;

    return layout.receive_request_count > static_cast<std::size_t>(INT_MAX) ||
                   layout.send_request_count > static_cast<std::size_t>(INT_MAX)
               ? BLITZAR_STATUS_INVALID_ARGUMENT
               : BLITZAR_STATUS_OK;
}

blitzar_status MpiGhostTransport::PrepareWireLayout(
    std::size_t local_bytes, std::size_t remote_peer_count, BeginLayout& layout) const noexcept
{
    if (!MpiGhostProtocol::ToWireSize(layout.receive_slots, layout.receive_wire_size)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (remote_peer_count != 0 &&
        local_bytes > std::numeric_limits<std::size_t>::max() / remote_peer_count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    layout.send_wire_size = local_bytes * remote_peer_count;

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiGhostTransport::PrepareRoundStorage(
    MpiGhostExchange::Impl& state, const BeginLayout& layout) const noexcept
{
    if (!MpiGhostProtocol::ResizeWithinCapacity(state.receive_wire, layout.receive_wire_size) ||
        !MpiGhostProtocol::ResizeWithinCapacity(
            state.receive_requests, layout.receive_request_count) ||
        !MpiGhostProtocol::ResizeWithinCapacity(state.send_requests, layout.send_request_count) ||
        !MpiGhostProtocol::ResizeWithinCapacity(
            state.receive_statuses, layout.receive_request_count) ||
        !MpiGhostProtocol::ResizeWithinCapacity(
            state.receive_chunks, layout.receive_request_count) ||
        layout.receive_wire_size > state.receive_capacity * ParticleWireBytes ||
        layout.receive_request_count > state.receive_requests.capacity() ||
        layout.send_request_count > state.send_requests.capacity()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::fill(state.receive_counts.begin(), state.receive_counts.end(), 0);
    std::fill(state.offsets.begin(), state.offsets.end(), 0);

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_parallel
