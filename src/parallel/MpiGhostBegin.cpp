#include "parallel/MpiGhostTransport.hpp"

#include "parallel/MpiGhostProtocol.hpp"
#include "parallel/MpiGhostState.hpp"
#include "parallel/MpiPacketTransport.hpp"
#include "parallel/MpiSessionNative.hpp"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <limits>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

blitzar_status MpiGhostTransport::Begin(
    std::span<const ParticlePacket> local, MpiGhostExchange& exchange) const noexcept
{
    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        exchange.impl_.reset();

        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    if (exchange.impl_ == nullptr) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    MpiGhostExchange::Impl& state = *exchange.impl_;
    std::size_t local_bytes = 0;

    blitzar_status preparation_status = PrepareLocal(local, state, local_bytes);
    blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    blitzar_status synchronization_status = collectives_.SynchronizeStatus(
        preparation_status, "MpiGhostTransport", "ghost-begin-prepare", global_status);

    if (synchronization_status != BLITZAR_STATUS_OK || global_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status : global_status;
    }

    const blitzar_status peer_count_status = GatherPeerCounts(local.size(), state);

    if (peer_count_status != BLITZAR_STATUS_OK) {
        return peer_count_status;
    }

    BeginLayout layout{};
    preparation_status = PrepareLayout(local.size(), local_bytes, state, layout);
    global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    synchronization_status = collectives_.SynchronizeStatus(
        preparation_status, "MpiGhostTransport", "ghost-capacity-prepare", global_status);

    if (synchronization_status != BLITZAR_STATUS_OK || global_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status : global_status;
    }

    return PostRequests(local, state, layout);
#else

    (void)local;
    (void)exchange;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiGhostTransport::PrepareLocal(
    std::span<const ParticlePacket> local, MpiGhostExchange::Impl& state,
    std::size_t& local_bytes) const noexcept
{
    const std::size_t peer_count = static_cast<std::size_t>(session_.Size());

    if (state.active || local.size() > static_cast<std::size_t>(INT_MAX) ||
        state.peer_counts.size() != peer_count || state.peer_capacities.size() != peer_count ||
        state.wire_offsets.size() != peer_count || state.receive_counts.size() != peer_count ||
        state.offsets.size() != peer_count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (local.size() > state.send_capacity ||
        !MpiGhostProtocol::ToWireSize(local.size(), local_bytes)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!MpiGhostProtocol::ResizeWithinCapacity(state.local_wire, local_bytes)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!ParticleWireCodec::Encode(local,
            std::span<std::byte>(state.local_wire.data(), state.local_wire.size()))) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiGhostTransport::GatherPeerCounts(
    std::size_t local_size, MpiGhostExchange::Impl& state) const noexcept
{
    const blitzar_status peer_count_status =
        packets_.AllGatherCounts(static_cast<int>(local_size), state.peer_counts);
    blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = collectives_.SynchronizeStatus(
        peer_count_status, "MpiGhostTransport", "ghost-capacity", global_status);

    return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status : global_status;
}

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

    std::size_t receive_slots = 0;

    std::fill(state.peer_capacities.begin(), state.peer_capacities.end(), 0);
    std::fill(state.wire_offsets.begin(), state.wire_offsets.end(), 0);

    for (std::size_t peer = 0; peer < peer_count; ++peer) {
        const int peer_count_value = state.peer_counts[peer];

        if (peer_count_value < 0) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        state.peer_capacities[peer] = static_cast<std::size_t>(peer_count_value);

        if (peer == static_cast<std::size_t>(session_.Rank())) {
            continue;
        }

        state.wire_offsets[peer] = receive_slots;
        const std::size_t peer_chunks =
            MpiGhostProtocol::ChunkCount(state.peer_capacities[peer]);

        if (receive_slots > std::numeric_limits<std::size_t>::max() -
                                state.peer_capacities[peer] ||
            layout.receive_request_count >
                std::numeric_limits<std::size_t>::max() - peer_chunks) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        receive_slots += state.peer_capacities[peer];
        layout.receive_request_count += peer_chunks;
    }

    const std::size_t local_chunks = MpiGhostProtocol::ChunkCount(local_size);

    if (remote_peer_count != 0 &&
        local_chunks > std::numeric_limits<std::size_t>::max() / remote_peer_count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    layout.send_request_count = local_chunks * remote_peer_count;

    if (layout.receive_request_count > static_cast<std::size_t>(INT_MAX) ||
        layout.send_request_count > static_cast<std::size_t>(INT_MAX) ||
        !MpiGhostProtocol::ToWireSize(receive_slots, layout.receive_wire_size)) {
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

blitzar_status MpiGhostTransport::PostRequests(std::span<const ParticlePacket> local,
    MpiGhostExchange::Impl& state, const BeginLayout& layout) const noexcept
{
    state.transfer = {layout.send_wire_size, layout.receive_wire_size};
    state.active = true;

    blitzar_status status = PostReceiveRequests(state, layout);

    if (status != BLITZAR_STATUS_OK) {
        AbortExchange(state);

        return status;
    }

    status = PostSendRequests(local, state, layout);

    if (status != BLITZAR_STATUS_OK) {
        AbortExchange(state);
    }

    return status;
}

blitzar_status MpiGhostTransport::PostReceiveRequests(
    MpiGhostExchange::Impl& state, const BeginLayout& layout) const noexcept
{
#if defined(BLITZAR_HAS_MPI)
    std::size_t request_index = 0;

    for (int peer = 0; peer < session_.Size(); ++peer) {
        if (peer == session_.Rank()) {
            continue;
        }

        const std::size_t peer_index = static_cast<std::size_t>(peer);
        std::size_t packet_offset = 0;

        while (packet_offset < state.peer_capacities[peer_index]) {
            const std::size_t chunk = std::min(
                state.peer_capacities[peer_index] - packet_offset, layout.chunk_packets);
            int bytes = 0;

            if (!MpiGhostProtocol::ToWireBytes(chunk, bytes) ||
                request_index >= state.receive_chunks.size() ||
                request_index >= state.receive_requests.size()) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            state.receive_chunks[request_index] = {peer_index, packet_offset};
            state.receive_requests[request_index] = MPI_REQUEST_NULL;

            if (MPI_Irecv(state.receive_wire.data() +
                              (state.wire_offsets[peer_index] + packet_offset) * ParticleWireBytes,
                    bytes, MPI_BYTE, peer, MpiGhostProtocol::DataTag,
                    session_.Native().communicator,
                    &state.receive_requests[request_index]) != MPI_SUCCESS) {
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            ++request_index;
            packet_offset += chunk;
        }
    }

    return request_index == layout.receive_request_count ? BLITZAR_STATUS_OK
                                                          : BLITZAR_STATUS_INTERNAL_ERROR;
#else

    (void)state;
    (void)layout;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiGhostTransport::PostSendRequests(std::span<const ParticlePacket> local,
    MpiGhostExchange::Impl& state, const BeginLayout& layout) const noexcept
{
#if defined(BLITZAR_HAS_MPI)
    std::size_t request_index = 0;

    for (int peer = 0; peer < session_.Size(); ++peer) {
        if (peer == session_.Rank()) {
            continue;
        }

        std::size_t packet_offset = 0;

        while (packet_offset < local.size()) {
            const std::size_t chunk = std::min(local.size() - packet_offset, layout.chunk_packets);
            int bytes = 0;

            if (!MpiGhostProtocol::ToWireBytes(chunk, bytes) ||
                request_index >= state.send_requests.size()) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            state.send_requests[request_index] = MPI_REQUEST_NULL;
            const std::byte* local_data = state.local_wire.data() +
                                          packet_offset * ParticleWireBytes;

            if (MPI_Isend(local_data, bytes, MPI_BYTE, peer, MpiGhostProtocol::DataTag,
                    session_.Native().communicator, &state.send_requests[request_index]) !=
                MPI_SUCCESS) {
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            ++request_index;
            packet_offset += chunk;
        }
    }

    return request_index == layout.send_request_count ? BLITZAR_STATUS_OK
                                                       : BLITZAR_STATUS_INTERNAL_ERROR;
#else

    (void)local;
    (void)state;
    (void)layout;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

} // namespace blitzar_parallel
