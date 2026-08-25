#include "parallel/MpiGhostTransport.hpp"

#include "parallel/MpiGhostProtocol.hpp"
#include "parallel/MpiGhostState.hpp"
#include "parallel/MpiSessionNative.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

blitzar_status MpiGhostTransport::Complete(
    MpiGhostExchange& exchange, PacketBuffer& ghosts) const noexcept
{
    ghosts.Clear();

    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)

    const bool exchange_active = exchange.impl_ != nullptr && exchange.impl_->active;
    blitzar_status global_active_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status active_synchronization_status = collectives_.SynchronizeStatus(
        exchange_active ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "MpiGhostTransport", "ghost-complete-preflight", global_active_status);

    if (active_synchronization_status != BLITZAR_STATUS_OK ||
        global_active_status != BLITZAR_STATUS_OK) {
        Abort(exchange);

        return active_synchronization_status != BLITZAR_STATUS_OK ? active_synchronization_status
                                                                  : global_active_status;
    }
    if (exchange.impl_ == nullptr || !exchange.impl_->active) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    MpiGhostExchange::Impl& state = *exchange.impl_;

    blitzar_status status =
        MpiGhostProtocol::WaitRequests(state.receive_requests, state.receive_statuses);

    if (status != BLITZAR_STATUS_OK) {
        AbortExchange(state);

        return status;
    }

    status = MpiGhostProtocol::WaitRequests(state.send_requests);

    if (status != BLITZAR_STATUS_OK) {
        AbortExchange(state);

        return status;
    }

    std::size_t total = 0;
    blitzar_status count_status = BLITZAR_STATUS_OK;

    std::fill(state.receive_counts.begin(), state.receive_counts.end(), 0);

    for (std::size_t request = 0; request < state.receive_chunks.size(); ++request) {
        int bytes = 0;

        if (MPI_Get_count(&state.receive_statuses[request], MPI_BYTE, &bytes) != MPI_SUCCESS ||
            bytes < 0 || bytes % static_cast<int>(ParticleWireBytes) != 0) {
            count_status = BLITZAR_STATUS_INTERNAL_ERROR;

            break;
        }

        const MpiGhostExchange::Impl::ReceiveChunk chunk = state.receive_chunks[request];
        const std::size_t packet_count = static_cast<std::size_t>(bytes) / ParticleWireBytes;
        std::size_t& peer_count = state.receive_counts[chunk.peer_index];

        if (packet_count > state.peer_capacities[chunk.peer_index] ||
            peer_count > state.peer_capacities[chunk.peer_index] - packet_count) {
            count_status = BLITZAR_STATUS_INTERNAL_ERROR;

            break;
        }

        peer_count += packet_count;
    }

    if (count_status == BLITZAR_STATUS_OK) {
        for (int peer = 0; peer < session_.Size(); ++peer) {
            const std::size_t peer_index = static_cast<std::size_t>(peer);
            const std::size_t count = state.receive_counts[peer_index];

            if (total > std::numeric_limits<std::size_t>::max() - count) {
                count_status = BLITZAR_STATUS_INTERNAL_ERROR;

                break;
            }

            state.offsets[peer_index] = total;
            total += count;
        }
    }

    blitzar_status global_count_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status count_synchronization_status = collectives_.SynchronizeStatus(
        count_status, "MpiGhostTransport", "ghost-count-prepare", global_count_status);

    if (count_synchronization_status != BLITZAR_STATUS_OK ||
        global_count_status != BLITZAR_STATUS_OK) {
        AbortExchange(state);

        return count_synchronization_status != BLITZAR_STATUS_OK ? count_synchronization_status
                                                                 : global_count_status;
    }

    blitzar_status preparation_status = BLITZAR_STATUS_OK;

    if (!ghosts.EnsureCapacity(total) || !ghosts.ResizeBounded(total)) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    blitzar_status global_preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = collectives_.SynchronizeStatus(
        preparation_status, "MpiGhostTransport", "ghost-data-prepare", global_preparation_status);

    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_preparation_status != BLITZAR_STATUS_OK) {
        AbortExchange(state);

        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status
                                                           : global_preparation_status;
    }

    blitzar_status decode_status = BLITZAR_STATUS_OK;

    for (std::size_t request = 0; request < state.receive_chunks.size(); ++request) {
        int bytes = 0;

        if (MPI_Get_count(&state.receive_statuses[request], MPI_BYTE, &bytes) != MPI_SUCCESS ||
            bytes < 0 || bytes % static_cast<int>(ParticleWireBytes) != 0) {
            decode_status = BLITZAR_STATUS_INVALID_ARGUMENT;

            break;
        }

        const MpiGhostExchange::Impl::ReceiveChunk chunk = state.receive_chunks[request];
        const std::size_t packet_count = static_cast<std::size_t>(bytes) / ParticleWireBytes;

        if (packet_count == 0) {
            continue;
        }

        const std::size_t source_offset =
            (state.wire_offsets[chunk.peer_index] + chunk.packet_offset) * ParticleWireBytes;
        const std::size_t target_offset = state.offsets[chunk.peer_index] + chunk.packet_offset;

        if (chunk.packet_offset > state.receive_counts[chunk.peer_index] ||
            packet_count > state.receive_counts[chunk.peer_index] - chunk.packet_offset ||
            !ParticleWireCodec::Decode(
                std::span<const std::byte>(state.receive_wire.data(), state.receive_wire.size())
                    .subspan(source_offset, packet_count * ParticleWireBytes),
                ghosts.View().subspan(target_offset, packet_count))) {
            decode_status = BLITZAR_STATUS_INVALID_ARGUMENT;

            break;
        }
    }

    blitzar_status global_decode_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status decode_synchronization_status = collectives_.SynchronizeStatus(
        decode_status, "MpiGhostTransport", "ghost-data-decode", global_decode_status);

    ClearExchange(state);

    return decode_synchronization_status != BLITZAR_STATUS_OK ? decode_synchronization_status
                                                              : global_decode_status;
#else

    (void)exchange;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

} // namespace blitzar_parallel
