#include "parallel/MpiExchange.hpp"

#include <algorithm>
#include <climits>
#include <limits>
#include <new>
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
    if (pack_status != BLITZAR_STATUS_OK) {
        return pack_status;
    }
    return context_.BeginGhostExchange(local_packets.View(), exchange);
}

blitzar_status MpiExchange::CompleteGhosts(
    MpiContext::GhostExchange& exchange,
    PacketBuffer& ghosts) const noexcept
{
    return context_.CompleteGhostExchange(exchange, ghosts);
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
    const blitzar_status pack_status =
        PackLocal(local_state, local_ids, local_packets);
    if (pack_status != BLITZAR_STATUS_OK) {
        return pack_status;
    }
    if (!context_.IsUsable()) {
        return context_.Status();
    }
    if (!context_.IsDistributed()) {
        try {
            received.Resize(local_packets.Size());
            std::copy(
                local_packets.View().begin(),
                local_packets.View().end(),
                received.View().begin());
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
    try {
        send_counts.assign(static_cast<std::size_t>(size), 0);
        receive_counts.assign(static_cast<std::size_t>(size), 0);
        send_offsets.assign(static_cast<std::size_t>(size), 0);
        receive_offsets.assign(static_cast<std::size_t>(size), 0);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    for (const ParticlePacket& packet : local_packets.View()) {
        const int owner = decomposition_.Owner(
            {packet.x, packet.y, packet.z}, packet.id);
        if (owner < 0 || owner >= size) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        int& count = send_counts[static_cast<std::size_t>(owner)];
        if (count == INT_MAX) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        ++count;
    }

    std::size_t send_total = 0;
    for (int peer = 0; peer < size; ++peer) {
        const std::size_t index = static_cast<std::size_t>(peer);
        send_offsets[index] = send_total;
        const std::size_t count = static_cast<std::size_t>(send_counts[index]);
        if (send_total > std::numeric_limits<std::size_t>::max() - count) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        send_total += count;
    }

    PacketBuffer send_ordered;
    try {
        send_ordered.Resize(send_total);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    std::vector<std::size_t> write_offsets;
    try {
        write_offsets = send_offsets;
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    for (const ParticlePacket& packet : local_packets.View()) {
        const int owner = decomposition_.Owner(
            {packet.x, packet.y, packet.z}, packet.id);
        send_ordered.View()[write_offsets[static_cast<std::size_t>(owner)]++] =
            packet;
    }

    blitzar_status status = context_.AllToAllCounts(
        send_counts,
        receive_counts);
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    std::size_t receive_total = 0;
    for (int peer = 0; peer < size; ++peer) {
        const std::size_t index = static_cast<std::size_t>(peer);
        receive_offsets[index] = receive_total;
        const int count = receive_counts[index];
        if (count < 0 || receive_total >
                                  std::numeric_limits<std::size_t>::max() -
                                      static_cast<std::size_t>(count)) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        receive_total += static_cast<std::size_t>(count);
    }
    try {
        received.Resize(receive_total);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    std::vector<int> send_displacements;
    std::vector<int> receive_displacements;
    try {
        send_displacements.assign(static_cast<std::size_t>(size), 0);
        receive_displacements.assign(static_cast<std::size_t>(size), 0);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    for (int peer = 0; peer < size; ++peer) {
        const std::size_t index = static_cast<std::size_t>(peer);
        if (!ToCount(send_offsets[index], send_displacements[index]) ||
            !ToCount(receive_offsets[index], receive_displacements[index])) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }
    return context_.AllToAllPackets(
        send_ordered.View(),
        send_counts,
        send_displacements,
        received.View(),
        receive_counts,
        receive_displacements);
}

blitzar_status MpiExchange::Gather(
    blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids,
    PacketBuffer& gathered) const noexcept
{
    gathered.Clear();
    PacketBuffer local_packets;
    const blitzar_status pack_status =
        PackLocal(local_state, local_ids, local_packets);
    if (pack_status != BLITZAR_STATUS_OK) {
        return pack_status;
    }
    if (!context_.IsUsable()) {
        return context_.Status();
    }
    if (!context_.IsDistributed()) {
        try {
            gathered.Resize(local_packets.Size());
            std::copy(
                local_packets.View().begin(),
                local_packets.View().end(),
                gathered.View().begin());
        } catch (const std::bad_alloc&) {
            return BLITZAR_STATUS_ALLOCATION_FAILURE;
        }
        return BLITZAR_STATUS_OK;
    }

    const int size = context_.Size();
    int local_count = 0;
    if (!ToCount(local_packets.Size(), local_count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    std::vector<int> counts;
    try {
        counts.assign(static_cast<std::size_t>(size), 0);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    blitzar_status status = context_.AllGatherCounts(local_count, counts);
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    std::size_t total = 0;
    std::vector<int> displacements;
    try {
        displacements.assign(static_cast<std::size_t>(size), 0);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    for (int peer = 0; peer < size; ++peer) {
        const std::size_t index = static_cast<std::size_t>(peer);
        if (!ToCount(total, displacements[index]) || counts[index] < 0 ||
            total > std::numeric_limits<std::size_t>::max() -
                        static_cast<std::size_t>(counts[index])) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        total += static_cast<std::size_t>(counts[index]);
    }
    try {
        gathered.Resize(total);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    return context_.AllGatherPackets(
        local_packets.View(),
        gathered.View(),
        counts,
        displacements);
}

}  // namespace blitzar_parallel
