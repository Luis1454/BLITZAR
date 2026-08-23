#include "parallel/MpiExchange.hpp"

#include <algorithm>
#include <climits>
#include <limits>
#include <new>
#include <stdexcept>
#include <vector>

namespace blitzar_parallel {

namespace {

[[nodiscard]] bool ToCount(std::size_t value, int& result) noexcept
{
    if (value > static_cast<std::size_t>(INT_MAX)) {
        return false;
    }

    result = static_cast<int>(value);

    return true;
}

[[nodiscard]] blitzar_status PackLocal(blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids, PacketBuffer& packets) noexcept
{
    if (!blitzar_core::IsValid(local_state) || local_ids.size() != local_state.count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!packets.ResizeBounded(local_state.count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    return ParticlePacker::Pack(local_state, local_ids, packets.View())
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

} // namespace

MpiExchangeWorkspace::MpiExchangeWorkspace(std::size_t packet_capacity, std::size_t peer_count)
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
    std::size_t packet_capacity)
    : context_(context), decomposition_(decomposition),
      workspace_(packet_capacity, static_cast<std::size_t>(context.Size()))
{
    capacity_status_ = context_.PrepareCapacity(packet_capacity, ghost_exchange_);
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
    const blitzar_status pack_status = PackLocal(local_state, local_ids, workspace_.local_packets);
    status = SynchronizeStatus(pack_status, "ghost-pack");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    status = context_.BeginGhostExchange(workspace_.local_packets.View(), exchange);
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
    blitzar_status local_status, const char* phase) const noexcept
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

blitzar_status MpiExchange::Migrate(blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids, PacketBuffer& received) const noexcept
{
    received.Clear();
    blitzar_status status = SynchronizeStatus(capacity_status_, "capacity");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    status = SynchronizeStatus(
        decomposition_.IsInitialized() ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "migrate-domain");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    status = SynchronizeStatus(decomposition_.ValidateState(local_state), "migrate-domain-state");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    status = SynchronizeStatus(
        PackLocal(local_state, local_ids, workspace_.local_packets), "migrate-pack");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    if (!context_.IsDistributed()) {
        if (!received.ResizeBounded(workspace_.local_packets.Size())) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        std::copy(workspace_.local_packets.View().begin(), workspace_.local_packets.View().end(),
            received.View().begin());
        return BLITZAR_STATUS_OK;
    }

    const int size = context_.Size();
    if (workspace_.send_counts.size() != static_cast<std::size_t>(size) ||
        workspace_.receive_counts.size() != static_cast<std::size_t>(size) ||
        workspace_.send_offsets.size() != static_cast<std::size_t>(size) ||
        workspace_.receive_offsets.size() != static_cast<std::size_t>(size) ||
        workspace_.send_displacements.size() != static_cast<std::size_t>(size) ||
        workspace_.receive_displacements.size() != static_cast<std::size_t>(size) ||
        workspace_.write_offsets.size() != static_cast<std::size_t>(size)) {
        return SynchronizeStatus(BLITZAR_STATUS_INVALID_ARGUMENT, "migrate-capacity");
    }
    std::vector<int>& send_counts = workspace_.send_counts;
    std::vector<int>& receive_counts = workspace_.receive_counts;
    std::vector<std::size_t>& send_offsets = workspace_.send_offsets;
    std::vector<std::size_t>& receive_offsets = workspace_.receive_offsets;
    std::vector<int>& send_displacements = workspace_.send_displacements;
    std::vector<int>& receive_displacements = workspace_.receive_displacements;
    std::vector<std::size_t>& write_offsets = workspace_.write_offsets;
    blitzar_status preparation_status = BLITZAR_STATUS_OK;
    std::fill(send_counts.begin(), send_counts.end(), 0);
    std::fill(receive_counts.begin(), receive_counts.end(), 0);
    std::fill(send_offsets.begin(), send_offsets.end(), 0);
    std::fill(receive_offsets.begin(), receive_offsets.end(), 0);

    if (preparation_status == BLITZAR_STATUS_OK) {
        for (const ParticlePacket& packet : workspace_.local_packets.View()) {
            const int owner = decomposition_.Owner({packet.x, packet.y, packet.z}, packet.id);
            if (owner < 0 || owner >= size) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                break;
            }
            int& count = send_counts[static_cast<std::size_t>(owner)];
            if (count == INT_MAX) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                break;
            }
            ++count;
        }
    }
    std::size_t send_total = 0;
    if (preparation_status == BLITZAR_STATUS_OK) {
        for (int peer = 0; peer < size; ++peer) {
            const std::size_t index = static_cast<std::size_t>(peer);
            send_offsets[index] = send_total;
            const std::size_t count = static_cast<std::size_t>(send_counts[index]);
            if (send_total > std::numeric_limits<std::size_t>::max() - count) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                break;
            }
            send_total += count;
        }
    }

    if (preparation_status == BLITZAR_STATUS_OK) {
        if (!workspace_.ordered_packets.ResizeBounded(send_total)) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        std::copy(send_offsets.begin(), send_offsets.end(), write_offsets.begin());
    }
    if (preparation_status == BLITZAR_STATUS_OK) {
        for (const ParticlePacket& packet : workspace_.local_packets.View()) {
            const int owner = decomposition_.Owner({packet.x, packet.y, packet.z}, packet.id);
            workspace_.ordered_packets.View()[write_offsets[static_cast<std::size_t>(owner)]++] =
                packet;
        }
    }

    status = SynchronizeStatus(preparation_status, "migrate-prepare");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = context_.AllToAllCounts(send_counts, receive_counts);
    status = SynchronizeStatus(status, "migrate-counts");
    if (status != BLITZAR_STATUS_OK) {
        received.Clear();
        return status;
    }

    std::size_t receive_total = 0;
    preparation_status = BLITZAR_STATUS_OK;
    for (int peer = 0; peer < size; ++peer) {
        const std::size_t index = static_cast<std::size_t>(peer);
        receive_offsets[index] = receive_total;
        const int count = receive_counts[index];
        if (count < 0 || receive_total > std::numeric_limits<std::size_t>::max() -
                                             static_cast<std::size_t>(count)) {
            preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;

            break;
        }
        receive_total += static_cast<std::size_t>(count);
    }

    if (preparation_status == BLITZAR_STATUS_OK) {
        if (!received.ResizeBounded(receive_total)) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }
    if (preparation_status == BLITZAR_STATUS_OK) {
        for (int peer = 0; peer < size; ++peer) {
            const std::size_t index = static_cast<std::size_t>(peer);
            if (!ToCount(send_offsets[index], send_displacements[index]) ||
                !ToCount(receive_offsets[index], receive_displacements[index])) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;

                break;
            }
        }
    }

    status = SynchronizeStatus(preparation_status, "migrate-packet-prepare");
    if (status != BLITZAR_STATUS_OK) {
        received.Clear();
        return status;
    }

    status = context_.AllToAllPackets(workspace_.ordered_packets.View(), send_counts,
        send_displacements, received.View(), receive_counts, receive_displacements);
    status = SynchronizeStatus(status, "migrate-packets");
    if (status != BLITZAR_STATUS_OK) {
        received.Clear();
    }
    return status;
}

blitzar_status MpiExchange::Gather(blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids, PacketBuffer& gathered) const noexcept
{
    gathered.Clear();
    blitzar_status status = SynchronizeStatus(capacity_status_, "capacity");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    status = SynchronizeStatus(
        PackLocal(local_state, local_ids, workspace_.local_packets), "gather-pack");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    if (!context_.IsDistributed()) {
        if (!gathered.ResizeBounded(workspace_.local_packets.Size())) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        std::copy(workspace_.local_packets.View().begin(), workspace_.local_packets.View().end(),
            gathered.View().begin());
        return BLITZAR_STATUS_OK;
    }

    const int size = context_.Size();
    int local_count = 0;
    if (workspace_.gather_counts.size() != static_cast<std::size_t>(size) ||
        workspace_.gather_displacements.size() != static_cast<std::size_t>(size)) {
        return SynchronizeStatus(BLITZAR_STATUS_INVALID_ARGUMENT, "gather-capacity");
    }
    std::vector<int>& counts = workspace_.gather_counts;
    std::vector<int>& displacements = workspace_.gather_displacements;
    blitzar_status preparation_status = BLITZAR_STATUS_OK;
    if (!ToCount(workspace_.local_packets.Size(), local_count)) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    std::fill(counts.begin(), counts.end(), 0);
    status = SynchronizeStatus(preparation_status, "gather-count-prepare");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = context_.AllGatherCounts(local_count, counts);
    status = SynchronizeStatus(status, "gather-counts");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    std::size_t total = 0;
    preparation_status = BLITZAR_STATUS_OK;
    std::fill(displacements.begin(), displacements.end(), 0);
    if (preparation_status == BLITZAR_STATUS_OK) {
        for (int peer = 0; peer < size; ++peer) {
            const std::size_t index = static_cast<std::size_t>(peer);
            if (!ToCount(total, displacements[index]) || counts[index] < 0 ||
                total > std::numeric_limits<std::size_t>::max() -
                            static_cast<std::size_t>(counts[index])) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;

                break;
            }
            total += static_cast<std::size_t>(counts[index]);
        }
    }
    if (preparation_status == BLITZAR_STATUS_OK) {
        if (!gathered.ResizeBounded(total)) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }
    status = SynchronizeStatus(preparation_status, "gather-packet-prepare");
    if (status != BLITZAR_STATUS_OK) {
        gathered.Clear();
        return status;
    }

    status = context_.AllGatherPackets(
        workspace_.local_packets.View(), gathered.View(), counts, displacements);
    status = SynchronizeStatus(status, "gather-packets");
    if (status != BLITZAR_STATUS_OK) {
        gathered.Clear();
    }
    return status;
}

} // namespace blitzar_parallel
