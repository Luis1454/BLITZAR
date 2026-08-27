#include "mpi/ghost/MpiGhostProtocol.hpp"
#include "mpi/ghost/MpiGhostState.hpp"
#include "mpi/ghost/MpiGhostTransport.hpp"

#include <algorithm>
#include <cstddef>

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
                request_index >= state.receive_chunks.size()) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            state.receive_chunks[request_index] = {peer_index, packet_offset};

            const NativeGhostReceiveRequest request{
                std::span<std::byte>(state.receive_wire.data(), state.receive_wire.size()),
                (state.wire_offsets[peer_index] + packet_offset) * ParticleWireBytes, bytes, peer,
                MpiGhostProtocol::DataTag};

            if (state.native == nullptr ||
                session_.Native().PostGhostReceive(*state.native, request) != BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            ++request_index;

            packet_offset += chunk;
        }
    }

    return request_index == layout.receive_request_count ? BLITZAR_STATUS_OK
                                                         : BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MpiGhostTransport::PostSendRequests(std::span<const ParticlePacket> local,
    MpiGhostExchange::Impl& state, const BeginLayout& layout) const noexcept
{
    std::size_t request_index = 0;

    for (int peer = 0; peer < session_.Size(); ++peer) {
        if (peer == session_.Rank()) {
            continue;
        }

        std::size_t packet_offset = 0;

        while (packet_offset < local.size()) {
            const std::size_t chunk = std::min(local.size() - packet_offset, layout.chunk_packets);
            int bytes = 0;

            if (!MpiGhostProtocol::ToWireBytes(chunk, bytes)) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            const NativeGhostSendRequest request{
                std::span<const std::byte>(state.local_wire.data(), state.local_wire.size()),
                packet_offset * ParticleWireBytes, bytes, peer, MpiGhostProtocol::DataTag};

            if (state.native == nullptr ||
                session_.Native().PostGhostSend(*state.native, request) != BLITZAR_STATUS_OK) {
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            ++request_index;

            packet_offset += chunk;
        }
    }

    return request_index == layout.send_request_count ? BLITZAR_STATUS_OK
                                                      : BLITZAR_STATUS_INTERNAL_ERROR;
}

} // namespace blitzar_parallel
