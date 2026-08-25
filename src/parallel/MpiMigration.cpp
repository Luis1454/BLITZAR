#include "parallel/MpiExchange.hpp"

#include <algorithm>
#include <climits>
#include <limits>
#include <vector>

namespace blitzar_parallel {

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

    status = SynchronizeStatus(
        decomposition_.ValidateState(local_state), "migrate-domain-state");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = SynchronizeStatus(
        PackLocal(local_state, local_ids, state_.local_packets), "migrate-pack");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    if (!context_.IsDistributed()) {
        if (!received.ResizeBounded(state_.local_packets.Size())) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        std::copy(state_.local_packets.View().begin(), state_.local_packets.View().end(),
            received.View().begin());

        return BLITZAR_STATUS_OK;
    }

    const int size = context_.Size();

    if (state_.send_counts.size() != static_cast<std::size_t>(size) ||
        state_.receive_counts.size() != static_cast<std::size_t>(size) ||
        state_.send_offsets.size() != static_cast<std::size_t>(size) ||
        state_.receive_offsets.size() != static_cast<std::size_t>(size) ||
        state_.send_displacements.size() != static_cast<std::size_t>(size) ||
        state_.receive_displacements.size() != static_cast<std::size_t>(size) ||
        state_.write_offsets.size() != static_cast<std::size_t>(size)) {
        return SynchronizeStatus(BLITZAR_STATUS_INVALID_ARGUMENT, "migrate-capacity");
    }

    std::vector<int>& send_counts = state_.send_counts;
    std::vector<int>& receive_counts = state_.receive_counts;
    std::vector<std::size_t>& send_offsets = state_.send_offsets;
    std::vector<std::size_t>& receive_offsets = state_.receive_offsets;
    std::vector<int>& send_displacements = state_.send_displacements;
    std::vector<int>& receive_displacements = state_.receive_displacements;
    std::vector<std::size_t>& write_offsets = state_.write_offsets;

    blitzar_status preparation_status = BLITZAR_STATUS_OK;

    std::fill(send_counts.begin(), send_counts.end(), 0);
    std::fill(receive_counts.begin(), receive_counts.end(), 0);
    std::fill(send_offsets.begin(), send_offsets.end(), 0);
    std::fill(receive_offsets.begin(), receive_offsets.end(), 0);

    for (const ParticlePacket& packet : state_.local_packets.View()) {
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
        if (!state_.ordered_packets.EnsureCapacity(send_total) ||
            !state_.ordered_packets.ResizeBounded(send_total)) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        std::copy(send_offsets.begin(), send_offsets.end(), write_offsets.begin());
    }
    if (preparation_status == BLITZAR_STATUS_OK) {
        for (const ParticlePacket& packet : state_.local_packets.View()) {
            const int owner = decomposition_.Owner({packet.x, packet.y, packet.z}, packet.id);

            state_.ordered_packets.View()[write_offsets[static_cast<std::size_t>(owner)]++] =
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
        if (!received.EnsureCapacity(receive_total) ||
            !received.ResizeBounded(receive_total)) {
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

    const AllToAllPacketRequest packet_request{
        state_.ordered_packets.View(), send_counts, send_displacements, received.View(),
        receive_counts, receive_displacements};

    status = context_.AllToAllPackets(packet_request);
    status = SynchronizeStatus(status, "migrate-packets");

    if (status != BLITZAR_STATUS_OK) {
        received.Clear();
    }

    return status;
}

} // namespace blitzar_parallel
