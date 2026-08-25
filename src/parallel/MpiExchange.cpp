#include "parallel/MpiExchange.hpp"

#include <algorithm>
#include <climits>
#include <limits>
#include <new>
#include <stdexcept>
#include <vector>

namespace blitzar_parallel {

bool MpiExchange::ToCount(std::size_t value, int& result) noexcept
{
    if (value > static_cast<std::size_t>(INT_MAX)) {
        return false;
    }

    result = static_cast<int>(value);

    return true;
}

blitzar_status MpiExchange::PackLocal(blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids, PacketBuffer& packets) noexcept
{
    if (!blitzar_core::IsValid(local_state) || local_ids.size() != local_state.count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!packets.EnsureCapacity(local_state.count) || !packets.ResizeBounded(local_state.count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return ParticlePacker::Pack(local_state, local_ids, packets.View())
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

ExchangeState::ExchangeState(std::size_t packet_capacity, std::size_t peer_count)
    : packet_capacity(packet_capacity), local_packets(), ordered_packets(),
      send_counts(peer_count, 0), receive_counts(peer_count, 0), send_displacements(peer_count, 0),
      receive_displacements(peer_count, 0), gather_counts(peer_count, 0),
      gather_displacements(peer_count, 0), send_offsets(peer_count, 0),
      receive_offsets(peer_count, 0), write_offsets(peer_count, 0)
{
    local_packets.Reserve(packet_capacity);
    ordered_packets.Reserve(packet_capacity);
}

MpiExchange::MpiExchange(const MpiContext& context, const DomainDecomposition& decomposition,
    std::size_t packet_capacity, std::size_t ghost_capacity)
    : context_(context), decomposition_(decomposition),
      state_(packet_capacity, static_cast<std::size_t>(context.Size()))
{
    const MpiContext::GhostCapacity capacities{
        packet_capacity, ghost_capacity == 0 ? packet_capacity : ghost_capacity};

    capacity_status_ = context_.PrepareCapacity(packet_capacity, ghost_exchange_, capacities);
}

MpiContext::GhostExchange& MpiExchange::PersistentGhostExchange() const noexcept
{
    return ghost_exchange_;
}

blitzar_status MpiExchange::BeginGhosts(blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids, MpiContext::GhostExchange& exchange) const noexcept
{
    blitzar_status status = SynchronizeStatus(capacity_status_, "capacity");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    const blitzar_status pack_status = PackLocal(local_state, local_ids, state_.local_packets);

    status = SynchronizeStatus(pack_status, "ghost-pack");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = context_.BeginGhostExchange(state_.local_packets.View(), exchange);
    status = SynchronizeStatus(status, "ghost-begin");

    if (status != BLITZAR_STATUS_OK) {
        context_.AbortGhostExchange(exchange);
    }

    return status;
}

blitzar_status MpiExchange::CompleteGhosts(
    MpiContext::GhostExchange& exchange, PacketBuffer& ghosts) const noexcept
{
    blitzar_status status = SynchronizeStatus(capacity_status_, "capacity");

    if (status != BLITZAR_STATUS_OK) {
        ghosts.Clear();

        return status;
    }

    status = context_.CompleteGhostExchange(exchange, ghosts);
    status = SynchronizeStatus(status, "ghost-complete");

    if (status != BLITZAR_STATUS_OK) {
        context_.AbortGhostExchange(exchange);
        ghosts.Clear();
    }

    return status;
}

void MpiExchange::AbortGhosts(
    MpiContext::GhostExchange& exchange, PacketBuffer& ghosts) const noexcept
{
    context_.AbortGhostExchange(exchange);
    ghosts.Clear();
}

blitzar_status MpiExchange::SynchronizeStatus(
    blitzar_status local_status, std::string_view phase) const noexcept
{
    blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        context_.SynchronizeStatus(local_status, "MpiExchange", phase, global_status);

    return synchronization_status == BLITZAR_STATUS_OK ? global_status : synchronization_status;
}

blitzar_status MpiExchange::ExchangeGhosts(blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids, PacketBuffer& ghosts) const noexcept
{
    MpiContext::GhostExchange& exchange = ghost_exchange_;

    const blitzar_status begin_status = BeginGhosts(local_state, local_ids, exchange);

    if (begin_status != BLITZAR_STATUS_OK) {
        ghosts.Clear();

        return begin_status;
    }

    return CompleteGhosts(exchange, ghosts);
}

} // namespace blitzar_parallel
