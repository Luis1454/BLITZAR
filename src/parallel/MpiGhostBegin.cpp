#include "parallel/MpiGhostProtocol.hpp"
#include "parallel/MpiGhostState.hpp"
#include "parallel/MpiGhostTransport.hpp"
#include "parallel/MpiPacketTransport.hpp"

#include <climits>
#include <cstddef>

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

blitzar_status MpiGhostTransport::PrepareLocal(std::span<const ParticlePacket> local,
    MpiGhostExchange::Impl& state, std::size_t& local_bytes) const noexcept
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
    if (!ParticleWireCodec::Encode(
            local, std::span<std::byte>(state.local_wire.data(), state.local_wire.size()))) {
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

} // namespace blitzar_parallel
