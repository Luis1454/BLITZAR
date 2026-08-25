#include "parallel/MpiGhostProtocol.hpp"
#include "parallel/MpiGhostState.hpp"
#include "parallel/MpiGhostTransport.hpp"
#include "parallel/MpiSessionNative.hpp"

#include <algorithm>
#include <cstddef>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

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
            const std::size_t chunk =
                std::min(state.peer_capacities[peer_index] - packet_offset, layout.chunk_packets);

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

            const std::byte* local_data =
                state.local_wire.data() + packet_offset * ParticleWireBytes;

            if (MPI_Isend(local_data, bytes, MPI_BYTE, peer, MpiGhostProtocol::DataTag,
                    session_.Native().communicator,
                    &state.send_requests[request_index]) != MPI_SUCCESS) {
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
