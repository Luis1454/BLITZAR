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

    MpiGhostExchange::Impl* state_pointer = exchange.impl_.get();

    blitzar_status preparation_status =
        state_pointer == nullptr ? BLITZAR_STATUS_INVALID_ARGUMENT : BLITZAR_STATUS_OK;

    if (state_pointer != nullptr && preparation_status == BLITZAR_STATUS_OK) {
        const MpiGhostExchange::Impl& state = *state_pointer;
        const std::size_t peer_count = static_cast<std::size_t>(session_.Size());

        if (state.active || local.size() > static_cast<std::size_t>(INT_MAX) ||
            state.peer_counts.size() != peer_count ||
            state.peer_capacities.size() != peer_count ||
            state.wire_offsets.size() != peer_count ||
            state.receive_counts.size() != peer_count || state.offsets.size() != peer_count) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    std::size_t local_bytes = 0;

    if (preparation_status == BLITZAR_STATUS_OK &&
        !MpiGhostProtocol::ToWireSize(local.size(), local_bytes)) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (preparation_status == BLITZAR_STATUS_OK) {
        MpiGhostExchange::Impl& state = *state_pointer;

        if (!MpiGhostProtocol::EnsureCapacity(state.local_wire, local_bytes) ||
            !MpiGhostProtocol::ResizeWithinCapacity(state.local_wire, local_bytes)) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        else if (!ParticleWireCodec::Encode(local,
                     std::span<std::byte>(state.local_wire.data(), state.local_wire.size()))) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    blitzar_status global_preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = collectives_.SynchronizeStatus(
        preparation_status, "MpiGhostTransport", "ghost-begin-prepare", global_preparation_status);

    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_preparation_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status
                                                           : global_preparation_status;
    }

    MpiGhostExchange::Impl& state = *state_pointer;

    const blitzar_status peer_count_status =
        packets_.AllGatherCounts(static_cast<int>(local.size()), state.peer_counts);

    blitzar_status global_peer_count_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status peer_count_synchronization_status = collectives_.SynchronizeStatus(
        peer_count_status, "MpiGhostTransport", "ghost-capacity", global_peer_count_status);

    if (peer_count_synchronization_status != BLITZAR_STATUS_OK ||
        global_peer_count_status != BLITZAR_STATUS_OK) {
        return peer_count_synchronization_status != BLITZAR_STATUS_OK
                   ? peer_count_synchronization_status
                   : global_peer_count_status;
    }

    const std::size_t peer_count = static_cast<std::size_t>(session_.Size());
    const std::size_t remote_peer_count = peer_count - 1;
    const std::size_t chunk_packets = MpiGhostProtocol::PointChunkPackets();
    std::size_t receive_slots = 0;
    std::size_t receive_request_count = 0;
    std::size_t send_request_count = 0;
    std::size_t receive_wire_size = 0;
    std::size_t send_wire_size = 0;

    preparation_status = BLITZAR_STATUS_OK;

    std::fill(state.peer_capacities.begin(), state.peer_capacities.end(), 0);
    std::fill(state.wire_offsets.begin(), state.wire_offsets.end(), 0);

    if (chunk_packets == 0) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t peer = 0; preparation_status == BLITZAR_STATUS_OK && peer < peer_count;
         ++peer) {
        const int peer_count_value = state.peer_counts[peer];

        if (peer_count_value < 0) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;

            break;
        }

        state.peer_capacities[peer] = static_cast<std::size_t>(peer_count_value);

        if (peer == static_cast<std::size_t>(session_.Rank())) {
            continue;
        }

        state.wire_offsets[peer] = receive_slots;

        if (receive_slots > std::numeric_limits<std::size_t>::max() -
                                state.peer_capacities[peer] ||
            receive_request_count >
                std::numeric_limits<std::size_t>::max() -
                    MpiGhostProtocol::ChunkCount(state.peer_capacities[peer])) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;

            break;
        }

        receive_slots += state.peer_capacities[peer];
        receive_request_count += MpiGhostProtocol::ChunkCount(state.peer_capacities[peer]);
    }

    if (preparation_status == BLITZAR_STATUS_OK && remote_peer_count != 0 &&
        MpiGhostProtocol::ChunkCount(local.size()) >
            std::numeric_limits<std::size_t>::max() / remote_peer_count) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    else if (preparation_status == BLITZAR_STATUS_OK) {
        send_request_count = MpiGhostProtocol::ChunkCount(local.size()) * remote_peer_count;
    }

    if (preparation_status == BLITZAR_STATUS_OK &&
        (receive_request_count > static_cast<std::size_t>(INT_MAX) ||
            send_request_count > static_cast<std::size_t>(INT_MAX) ||
            !MpiGhostProtocol::ToWireSize(receive_slots, receive_wire_size))) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (preparation_status == BLITZAR_STATUS_OK && remote_peer_count != 0 &&
        local_bytes > std::numeric_limits<std::size_t>::max() / remote_peer_count) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    else if (preparation_status == BLITZAR_STATUS_OK) {
        send_wire_size = local_bytes * remote_peer_count;
    }

    if (preparation_status == BLITZAR_STATUS_OK &&
        (!MpiGhostProtocol::EnsureCapacity(state.receive_wire, receive_wire_size) ||
            !MpiGhostProtocol::ResizeWithinCapacity(state.receive_wire, receive_wire_size) ||
            !MpiGhostProtocol::EnsureCapacity(
                state.receive_requests, receive_request_count) ||
            !MpiGhostProtocol::EnsureCapacity(state.send_requests, send_request_count) ||
            !MpiGhostProtocol::EnsureCapacity(
                state.receive_statuses, receive_request_count) ||
            !MpiGhostProtocol::EnsureCapacity(state.receive_chunks, receive_request_count) ||
            !MpiGhostProtocol::ResizeWithinCapacity(
                state.receive_requests, receive_request_count) ||
            !MpiGhostProtocol::ResizeWithinCapacity(state.send_requests, send_request_count) ||
            !MpiGhostProtocol::ResizeWithinCapacity(
                state.receive_statuses, receive_request_count) ||
            !MpiGhostProtocol::ResizeWithinCapacity(
                state.receive_chunks, receive_request_count))) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (preparation_status == BLITZAR_STATUS_OK) {
        std::fill(state.receive_counts.begin(), state.receive_counts.end(), 0);
        std::fill(state.offsets.begin(), state.offsets.end(), 0);
    }

    blitzar_status global_capacity_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status capacity_synchronization_status = collectives_.SynchronizeStatus(
        preparation_status, "MpiGhostTransport", "ghost-capacity-prepare", global_capacity_status);

    if (capacity_synchronization_status != BLITZAR_STATUS_OK ||
        global_capacity_status != BLITZAR_STATUS_OK) {
        return capacity_synchronization_status != BLITZAR_STATUS_OK
                   ? capacity_synchronization_status
                   : global_capacity_status;
    }

    state.transfer = {send_wire_size, receive_wire_size};
    state.active = true;

    std::size_t receive_request_index = 0;
    std::size_t send_request_index = 0;

    for (int peer = 0; peer < session_.Size(); ++peer) {
        if (peer == session_.Rank()) {
            continue;
        }

        const std::size_t peer_index = static_cast<std::size_t>(peer);

        std::size_t packet_offset = 0;

        while (packet_offset < state.peer_capacities[peer_index]) {
            const std::size_t chunk = std::min(
                state.peer_capacities[peer_index] - packet_offset, chunk_packets);

            int bytes = 0;

            if (!MpiGhostProtocol::ToWireBytes(chunk, bytes)) {
                AbortExchange(state);

                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            if (receive_request_index >= state.receive_chunks.size() ||
                receive_request_index >= state.receive_requests.size()) {
                AbortExchange(state);

                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            state.receive_chunks[receive_request_index] = {peer_index, packet_offset};
            state.receive_requests[receive_request_index] = MPI_REQUEST_NULL;

            if (MPI_Irecv(state.receive_wire.data() +
                              (state.wire_offsets[peer_index] + packet_offset) * ParticleWireBytes,
                    bytes, MPI_BYTE, peer, MpiGhostProtocol::DataTag,
                    session_.Native().communicator,
                    &state.receive_requests[receive_request_index]) != MPI_SUCCESS) {
                AbortExchange(state);

                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            ++receive_request_index;

            packet_offset += chunk;
        }

        packet_offset = 0;

        while (packet_offset < local.size()) {
            const std::size_t chunk = std::min(local.size() - packet_offset, chunk_packets);

            int local_bytes_count = 0;

            if (!MpiGhostProtocol::ToWireBytes(chunk, local_bytes_count)) {
                AbortExchange(state);

                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            if (send_request_index >= state.send_requests.size()) {
                AbortExchange(state);

                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            state.send_requests[send_request_index] = MPI_REQUEST_NULL;

            const std::byte* local_data = state.local_wire.data() +
                                          packet_offset * ParticleWireBytes;

            if (MPI_Isend(local_data, local_bytes_count, MPI_BYTE, peer, MpiGhostProtocol::DataTag,
                    session_.Native().communicator, &state.send_requests[send_request_index]) !=
                MPI_SUCCESS) {
                AbortExchange(state);

                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            ++send_request_index;

            packet_offset += chunk;
        }
    }

    if (receive_request_index != receive_request_count ||
        send_request_index != send_request_count) {
        AbortExchange(state);

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return BLITZAR_STATUS_OK;
#else

    (void)local;
    (void)exchange;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

} // namespace blitzar_parallel
