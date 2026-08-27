#include "mpi/ghost/MpiGhostProtocol.hpp"
#include "mpi/ghost/MpiGhostState.hpp"
#include "mpi/ghost/MpiGhostTransport.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace blitzar_parallel {

blitzar_status MpiGhostTransport::CountReceived(
    MpiGhostExchange::Impl& state, std::size_t& total) const noexcept
{
    total = 0;

    std::fill(state.receive_counts.begin(), state.receive_counts.end(), 0);

    for (std::size_t request = 0; request < state.receive_chunks.size(); ++request) {
        const std::size_t bytes = state.receive_byte_counts[request];

        if (bytes % ParticleWireBytes != 0) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        const MpiGhostExchange::Impl::ReceiveChunk chunk = state.receive_chunks[request];
        const std::size_t packet_count = bytes / ParticleWireBytes;

        std::size_t& peer_count = state.receive_counts[chunk.peer_index];

        if (packet_count > state.peer_capacities[chunk.peer_index] ||
            peer_count > state.peer_capacities[chunk.peer_index] - packet_count) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        peer_count += packet_count;
    }

    for (int peer = 0; peer < session_.Size(); ++peer) {
        const std::size_t peer_index = static_cast<std::size_t>(peer);
        const std::size_t count = state.receive_counts[peer_index];

        if (total > std::numeric_limits<std::size_t>::max() - count) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        state.offsets[peer_index] = total;
        total += count;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiGhostTransport::PrepareGhostBuffer(
    std::size_t total, PacketBuffer& ghosts) const noexcept
{
    return ghosts.EnsureCapacity(total) && ghosts.ResizeBounded(total)
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

blitzar_status MpiGhostTransport::DecodeReceived(
    MpiGhostExchange::Impl& state, PacketBuffer& ghosts) const noexcept
{
    for (std::size_t request = 0; request < state.receive_chunks.size(); ++request) {
        const std::size_t bytes = state.receive_byte_counts[request];

        if (bytes % ParticleWireBytes != 0) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const MpiGhostExchange::Impl::ReceiveChunk chunk = state.receive_chunks[request];
        const std::size_t packet_count = bytes / ParticleWireBytes;

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
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_parallel
