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

[[nodiscard]] blitzar_status PackLocal(
    blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids,
    PacketBuffer& packets) noexcept
{
    if (!blitzar_core::IsValid(local_state) ||
        local_ids.size() != local_state.count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    try {
        packets.Resize(local_state.count);
    } catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    return ParticlePacker::Pack(local_state, local_ids, packets.View())
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

}  // namespace

MpiExchange::MpiExchange(
    const MpiContext& context,
    const DomainDecomposition& decomposition) noexcept
    : context_(context), decomposition_(decomposition)
{
}

blitzar_status MpiExchange::BeginGhosts(
    blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids,
    MpiContext::GhostExchange& exchange) const noexcept
{
    PacketBuffer local_packets;
    const blitzar_status pack_status =
        PackLocal(local_state, local_ids, local_packets);
    blitzar_status status = SynchronizeStatus(pack_status, "ghost-pack");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    status = context_.BeginGhostExchange(local_packets.View(), exchange);
    status = SynchronizeStatus(status, "ghost-begin");
    if (status != BLITZAR_STATUS_OK) {
        context_.AbortGhostExchange(exchange);
    }
    return status;
}

blitzar_status MpiExchange::CompleteGhosts(
    MpiContext::GhostExchange& exchange,
    PacketBuffer& ghosts) const noexcept
{
    blitzar_status status = context_.CompleteGhostExchange(exchange, ghosts);
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
    const blitzar_status synchronization_status = context_.SynchronizeStatus(
        local_status,
        "MpiExchange",
        phase,
        global_status);
    return synchronization_status == BLITZAR_STATUS_OK
               ? global_status
               : synchronization_status;
}

blitzar_status MpiExchange::ExchangeGhosts(
    blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids,
    PacketBuffer& ghosts) const noexcept
{
    MpiContext::GhostExchange exchange;
    const blitzar_status begin_status =
        BeginGhosts(local_state, local_ids, exchange);
    if (begin_status != BLITZAR_STATUS_OK) {
        ghosts.Clear();
        return begin_status;
    }
    return CompleteGhosts(exchange, ghosts);
}

blitzar_status MpiExchange::Migrate(
    blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids,
    PacketBuffer& received) const noexcept
{
    received.Clear();
    PacketBuffer local_packets;
    blitzar_status status = SynchronizeStatus(
        decomposition_.IsInitialized() ? BLITZAR_STATUS_OK
                                        : BLITZAR_STATUS_INVALID_ARGUMENT,
        "migrate-domain");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    status = SynchronizeStatus(
        PackLocal(local_state, local_ids, local_packets),
        "migrate-pack");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    if (!context_.IsDistributed()) {
        try {
            received.Resize(local_packets.Size());
            std::copy(
                local_packets.View().begin(),
                local_packets.View().end(),
                received.View().begin());
        } catch (const std::length_error&) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        } catch (const std::bad_alloc&) {
            return BLITZAR_STATUS_ALLOCATION_FAILURE;
        }
        return BLITZAR_STATUS_OK;
    }

    const int size = context_.Size();
    std::vector<int> send_counts;
    std::vector<int> receive_counts;
    std::vector<std::size_t> send_offsets;
    std::vector<std::size_t> receive_offsets;
    blitzar_status preparation_status = BLITZAR_STATUS_OK;
    try {
        send_counts.assign(static_cast<std::size_t>(size), 0);
        receive_counts.assign(static_cast<std::size_t>(size), 0);
        send_offsets.assign(static_cast<std::size_t>(size), 0);
        receive_offsets.assign(static_cast<std::size_t>(size), 0);
    } catch (const std::length_error&) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    if (preparation_status == BLITZAR_STATUS_OK) {
        for (const ParticlePacket& packet : local_packets.View()) {
            const int owner = decomposition_.Owner(
                {packet.x, packet.y, packet.z}, packet.id);
            if (owner < 0 || owner >= size) {
                preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
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
            const std::size_t count =
                static_cast<std::size_t>(send_counts[index]);
            if (send_total > std::numeric_limits<std::size_t>::max() - count) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                break;
            }
            send_total += count;
        }
    }

    PacketBuffer send_ordered;
    std::vector<std::size_t> write_offsets;
    if (preparation_status == BLITZAR_STATUS_OK) {
        try {
            send_ordered.Resize(send_total);
            write_offsets = send_offsets;
        } catch (const std::length_error&) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        } catch (const std::bad_alloc&) {
            preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
        }
    }
    if (preparation_status == BLITZAR_STATUS_OK) {
        for (const ParticlePacket& packet : local_packets.View()) {
            const int owner = decomposition_.Owner(
                {packet.x, packet.y, packet.z}, packet.id);
            send_ordered.View()[
                write_offsets[static_cast<std::size_t>(owner)]++] = packet;
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
        if (count < 0 || receive_total >
                                  std::numeric_limits<std::size_t>::max() -
                                      static_cast<std::size_t>(count)) {
            preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
            break;
        }
        receive_total += static_cast<std::size_t>(count);
    }

    std::vector<int> send_displacements;
    std::vector<int> receive_displacements;
    if (preparation_status == BLITZAR_STATUS_OK) {
        try {
            received.Resize(receive_total);
            send_displacements.assign(static_cast<std::size_t>(size), 0);
            receive_displacements.assign(static_cast<std::size_t>(size), 0);
        } catch (const std::length_error&) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        } catch (const std::bad_alloc&) {
            preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
        }
    }
    if (preparation_status == BLITZAR_STATUS_OK) {
        for (int peer = 0; peer < size; ++peer) {
            const std::size_t index = static_cast<std::size_t>(peer);
            if (!ToCount(send_offsets[index], send_displacements[index]) ||
                !ToCount(
                    receive_offsets[index], receive_displacements[index])) {
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

    status = context_.AllToAllPackets(
        send_ordered.View(),
        send_counts,
        send_displacements,
        received.View(),
        receive_counts,
        receive_displacements);
    status = SynchronizeStatus(status, "migrate-packets");
    if (status != BLITZAR_STATUS_OK) {
        received.Clear();
    }
    return status;
}

blitzar_status MpiExchange::Gather(
    blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids,
    PacketBuffer& gathered) const noexcept
{
    gathered.Clear();
    PacketBuffer local_packets;
    blitzar_status status = SynchronizeStatus(
        PackLocal(local_state, local_ids, local_packets),
        "gather-pack");
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    if (!context_.IsDistributed()) {
        try {
            gathered.Resize(local_packets.Size());
            std::copy(
                local_packets.View().begin(),
                local_packets.View().end(),
                gathered.View().begin());
        } catch (const std::length_error&) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        } catch (const std::bad_alloc&) {
            return BLITZAR_STATUS_ALLOCATION_FAILURE;
        }
        return BLITZAR_STATUS_OK;
    }

    const int size = context_.Size();
    int local_count = 0;
    std::vector<int> counts;
    blitzar_status preparation_status = BLITZAR_STATUS_OK;
    if (!ToCount(local_packets.Size(), local_count)) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    try {
        if (preparation_status == BLITZAR_STATUS_OK) {
            counts.assign(static_cast<std::size_t>(size), 0);
        }
    } catch (const std::length_error&) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
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
    std::vector<int> displacements;
    preparation_status = BLITZAR_STATUS_OK;
    try {
        displacements.assign(static_cast<std::size_t>(size), 0);
    } catch (const std::length_error&) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
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
        try {
            gathered.Resize(total);
        } catch (const std::length_error&) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        } catch (const std::bad_alloc&) {
            preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
        }
    }
    status = SynchronizeStatus(preparation_status, "gather-packet-prepare");
    if (status != BLITZAR_STATUS_OK) {
        gathered.Clear();
        return status;
    }

    status = context_.AllGatherPackets(
        local_packets.View(),
        gathered.View(),
        counts,
        displacements);
    status = SynchronizeStatus(status, "gather-packets");
    if (status != BLITZAR_STATUS_OK) {
        gathered.Clear();
    }
    return status;
}

}  // namespace blitzar_parallel
