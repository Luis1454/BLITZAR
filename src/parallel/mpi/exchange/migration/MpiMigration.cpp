#include "parallel/mpi/exchange/MpiExchange.hpp"

#include <algorithm>
#include <climits>
#include <limits>
#include <vector>

namespace blitzar_parallel {

blitzar_status MpiExchange::Migrate(blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids, PacketBuffer& received) const noexcept
{
    migration_trace_ = {};
    migration_trace_.local_before = local_state.count;

    received.Clear();

    blitzar_status status = SynchronizeStatus(capacity_status_, "capacity");

    if (status != BLITZAR_STATUS_OK) {
        migration_trace_.status = status;

        return status;
    }

    status = SynchronizeStatus(
        decomposition_.IsInitialized() ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "migrate-domain");

    if (status != BLITZAR_STATUS_OK) {
        migration_trace_.status = status;

        return status;
    }

    status = SynchronizeStatus(decomposition_.ValidateState(local_state), "migrate-domain-state");

    if (status != BLITZAR_STATUS_OK) {
        migration_trace_.status = status;

        return status;
    }

    status =
        SynchronizeStatus(PackLocal(local_state, local_ids, state_.local_packets), "migrate-pack");

    if (status != BLITZAR_STATUS_OK) {
        migration_trace_.status = status;

        return status;
    }
    if (!context_.IsDistributed()) {
        if (!received.ResizeBounded(state_.local_packets.Size())) {
            migration_trace_.status = BLITZAR_STATUS_INVALID_ARGUMENT;

            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        std::copy(state_.local_packets.View().begin(), state_.local_packets.View().end(),
            received.View().begin());

        migration_trace_.local_after = received.Size();
        migration_trace_.observed = migration_trace_.local_before != migration_trace_.local_after;

        return BLITZAR_STATUS_OK;
    }

    status = SynchronizeStatus(PrepareMigrationSend(), "migrate-prepare");

    if (status != BLITZAR_STATUS_OK) {
        migration_trace_.status = status;

        return status;
    }

    status = context_.AllToAllCounts(state_.send_counts, state_.receive_counts);
    status = SynchronizeStatus(status, "migrate-counts");

    if (status != BLITZAR_STATUS_OK) {
        received.Clear();

        migration_trace_.status = status;

        return status;
    }

    status = SynchronizeStatus(PrepareMigrationReceive(received), "migrate-packet-prepare");

    if (status != BLITZAR_STATUS_OK) {
        received.Clear();

        migration_trace_.status = status;

        return status;
    }

    const std::size_t local_rank = static_cast<std::size_t>(context_.Rank());

    for (std::size_t peer = 0; peer < state_.send_counts.size(); ++peer) {
        if (peer != local_rank) {
            migration_trace_.sent_remote += static_cast<std::size_t>(state_.send_counts[peer]);
            migration_trace_.received_remote +=
                static_cast<std::size_t>(state_.receive_counts[peer]);
        }
    }

    status = ExchangeMigrationPackets(received);
    migration_trace_.status = status;
    migration_trace_.local_after = received.Size();
    migration_trace_.observed =
        migration_trace_.sent_remote != 0 || migration_trace_.received_remote != 0;

    return status;
}

blitzar_status MpiExchange::PrepareMigrationSend() const noexcept
{
    const std::size_t peer_count = static_cast<std::size_t>(context_.Size());

    if (state_.send_counts.size() != peer_count || state_.receive_counts.size() != peer_count ||
        state_.send_offsets.size() != peer_count || state_.receive_offsets.size() != peer_count ||
        state_.send_displacements.size() != peer_count ||
        state_.receive_displacements.size() != peer_count ||
        state_.write_offsets.size() != peer_count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::fill(state_.send_counts.begin(), state_.send_counts.end(), 0);
    std::fill(state_.receive_counts.begin(), state_.receive_counts.end(), 0);
    std::fill(state_.send_offsets.begin(), state_.send_offsets.end(), 0);
    std::fill(state_.receive_offsets.begin(), state_.receive_offsets.end(), 0);

    const blitzar_status count_status = CountMigrationDestinations();

    return count_status == BLITZAR_STATUS_OK ? OrderMigrationPackets() : count_status;
}

blitzar_status MpiExchange::CountMigrationDestinations() const noexcept
{
    const int size = context_.Size();

    for (const ParticlePacket& packet : state_.local_packets.View()) {
        const int owner = decomposition_.Owner({packet.x, packet.y, packet.z}, packet.id);

        if (owner < 0 || owner >= size) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        int& count = state_.send_counts[static_cast<std::size_t>(owner)];

        if (count == INT_MAX) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        ++count;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiExchange::OrderMigrationPackets() const noexcept
{
    std::size_t send_total = 0;
    const int size = context_.Size();

    for (int peer = 0; peer < size; ++peer) {
        const std::size_t index = static_cast<std::size_t>(peer);

        state_.send_offsets[index] = send_total;

        const std::size_t count = static_cast<std::size_t>(state_.send_counts[index]);

        if (send_total > std::numeric_limits<std::size_t>::max() - count) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        send_total += count;
    }

    if (!state_.ordered_packets.ResizeBounded(send_total)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::copy(state_.send_offsets.begin(), state_.send_offsets.end(), state_.write_offsets.begin());

    for (const ParticlePacket& packet : state_.local_packets.View()) {
        const int owner = decomposition_.Owner({packet.x, packet.y, packet.z}, packet.id);

        state_.ordered_packets.View()[state_.write_offsets[static_cast<std::size_t>(owner)]++] =
            packet;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiExchange::PrepareMigrationReceive(PacketBuffer& received) const noexcept
{
    std::size_t receive_total = 0;
    const int size = context_.Size();

    for (int peer = 0; peer < size; ++peer) {
        const std::size_t index = static_cast<std::size_t>(peer);
        const int count = state_.receive_counts[index];

        state_.receive_offsets[index] = receive_total;

        if (count < 0 || receive_total > std::numeric_limits<std::size_t>::max() -
                                             static_cast<std::size_t>(count)) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        receive_total += static_cast<std::size_t>(count);
    }

    if (!received.ResizeBounded(receive_total)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (int peer = 0; peer < size; ++peer) {
        const std::size_t index = static_cast<std::size_t>(peer);

        if (!ToCount(state_.send_offsets[index], state_.send_displacements[index]) ||
            !ToCount(state_.receive_offsets[index], state_.receive_displacements[index])) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiExchange::ExchangeMigrationPackets(PacketBuffer& received) const noexcept
{
    const AllToAllPacketRequest request{state_.ordered_packets.View(), state_.send_counts,
        state_.send_displacements, received.View(), state_.receive_counts,
        state_.receive_displacements};

    const blitzar_status status = context_.AllToAllPackets(request);
    const blitzar_status synchronized_status = SynchronizeStatus(status, "migrate-packets");

    if (synchronized_status != BLITZAR_STATUS_OK) {
        received.Clear();
    }

    return synchronized_status;
}

} // namespace blitzar_parallel
