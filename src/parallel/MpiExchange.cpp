#include "parallel/MpiExchange.hpp"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <limits>
#include <new>
#include <vector>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

namespace {

constexpr int GhostCountTag = 7101;
constexpr int GhostDataTag = 7102;

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

#if defined(BLITZAR_HAS_MPI)
[[nodiscard]] bool ToMpiCount(
    std::size_t value, int& result) noexcept
{
    if (value > static_cast<std::size_t>(INT_MAX)) {
        return false;
    }
    result = static_cast<int>(value);
    return true;
}

[[nodiscard]] bool ToByteCount(
    std::size_t packet_count, int& result) noexcept
{
    if (packet_count > static_cast<std::size_t>(INT_MAX) /
                          sizeof(ParticlePacket)) {
        return false;
    }
    result = static_cast<int>(packet_count * sizeof(ParticlePacket));
    return true;
}

[[nodiscard]] blitzar_status WaitAll(std::vector<MPI_Request>& requests) noexcept
{
    if (requests.empty()) {
        return BLITZAR_STATUS_OK;
    }
    return MPI_Waitall(
               static_cast<int>(requests.size()),
               requests.data(),
               MPI_STATUSES_IGNORE) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}
#endif

}  // namespace

MpiExchange::MpiExchange(
    const MpiContext& context,
    const DomainDecomposition& decomposition) noexcept
    : context_(context), decomposition_(decomposition)
{
}

blitzar_status MpiExchange::ExchangeGhosts(
    blitzar_core::ParticleStateView local_state,
    std::span<const std::uint64_t> local_ids,
    PacketBuffer& ghosts) const noexcept
{
    ghosts.Clear();
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
        return BLITZAR_STATUS_OK;
    }

#if defined(BLITZAR_HAS_MPI)
    const int rank = context_.Rank();
    const int size = context_.Size();
    std::vector<int> receive_counts;
    std::vector<MPI_Request> count_requests;
    try {
        receive_counts.assign(static_cast<std::size_t>(size), 0);
        count_requests.reserve(static_cast<std::size_t>(size - 1) * 2);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    int local_count = 0;
    if (!ToMpiCount(local_packets.Size(), local_count)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    for (int peer = 0; peer < size; ++peer) {
        if (peer == rank) {
            continue;
        }
        count_requests.push_back(MPI_REQUEST_NULL);
        if (MPI_Irecv(
                &receive_counts[static_cast<std::size_t>(peer)],
                1,
                MPI_INT,
                peer,
                GhostCountTag,
                context_.Communicator(),
                &count_requests.back()) != MPI_SUCCESS) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        count_requests.push_back(MPI_REQUEST_NULL);
        if (MPI_Isend(
                &local_count,
                1,
                MPI_INT,
                peer,
                GhostCountTag,
                context_.Communicator(),
                &count_requests.back()) != MPI_SUCCESS) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
    }
    blitzar_status status = WaitAll(count_requests);
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    std::vector<std::size_t> offsets;
    std::size_t total = 0;
    try {
        offsets.assign(static_cast<std::size_t>(size), 0);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    for (int peer = 0; peer < size; ++peer) {
        const int count = receive_counts[static_cast<std::size_t>(peer)];
        if (count < 0 ||
            total > std::numeric_limits<std::size_t>::max() -
                        static_cast<std::size_t>(count)) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        offsets[static_cast<std::size_t>(peer)] = total;
        total += static_cast<std::size_t>(count);
    }
    try {
        ghosts.Resize(total);
        count_requests.clear();
        count_requests.reserve(static_cast<std::size_t>(size - 1) * 2);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    for (int peer = 0; peer < size; ++peer) {
        if (peer == rank) {
            continue;
        }
        int receive_bytes = 0;
        if (!ToByteCount(
                static_cast<std::size_t>(receive_counts[static_cast<std::size_t>(peer)]),
                receive_bytes)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        count_requests.push_back(MPI_REQUEST_NULL);
        if (MPI_Irecv(
                ghosts.Data() + offsets[static_cast<std::size_t>(peer)],
                receive_bytes,
                MPI_BYTE,
                peer,
                GhostDataTag,
                context_.Communicator(),
                &count_requests.back()) != MPI_SUCCESS) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        int send_bytes = 0;
        if (!ToByteCount(local_packets.Size(), send_bytes)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        count_requests.push_back(MPI_REQUEST_NULL);
        if (MPI_Isend(
                local_packets.Data(),
                send_bytes,
                MPI_BYTE,
                peer,
                GhostDataTag,
                context_.Communicator(),
                &count_requests.back()) != MPI_SUCCESS) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
    }
    return WaitAll(count_requests);
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
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

#if defined(BLITZAR_HAS_MPI)
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
        send_offsets[static_cast<std::size_t>(peer)] = send_total;
        const std::size_t count = static_cast<std::size_t>(
            send_counts[static_cast<std::size_t>(peer)]);
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
        send_ordered.View()[write_offsets[static_cast<std::size_t>(owner)]++] = packet;
    }
    if (MPI_Alltoall(
            send_counts.data(),
            1,
            MPI_INT,
            receive_counts.data(),
            1,
            MPI_INT,
            context_.Communicator()) != MPI_SUCCESS) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    std::size_t receive_total = 0;
    for (int peer = 0; peer < size; ++peer) {
        receive_offsets[static_cast<std::size_t>(peer)] = receive_total;
        const int count = receive_counts[static_cast<std::size_t>(peer)];
        if (count < 0 ||
            receive_total > std::numeric_limits<std::size_t>::max() -
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
    std::vector<int> send_bytes;
    std::vector<int> receive_bytes;
    std::vector<int> send_displacements;
    std::vector<int> receive_displacements;
    try {
        send_bytes.assign(static_cast<std::size_t>(size), 0);
        receive_bytes.assign(static_cast<std::size_t>(size), 0);
        send_displacements.assign(static_cast<std::size_t>(size), 0);
        receive_displacements.assign(static_cast<std::size_t>(size), 0);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    for (int peer = 0; peer < size; ++peer) {
        if (!ToByteCount(
                static_cast<std::size_t>(send_counts[static_cast<std::size_t>(peer)]),
                send_bytes[static_cast<std::size_t>(peer)]) ||
            !ToByteCount(
                static_cast<std::size_t>(receive_counts[static_cast<std::size_t>(peer)]),
                receive_bytes[static_cast<std::size_t>(peer)]) ||
            !ToByteCount(
                send_offsets[static_cast<std::size_t>(peer)],
                send_displacements[static_cast<std::size_t>(peer)]) ||
            !ToByteCount(
                receive_offsets[static_cast<std::size_t>(peer)],
                receive_displacements[static_cast<std::size_t>(peer)])) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }
    if (MPI_Alltoallv(
            send_ordered.Data(),
            send_bytes.data(),
            send_displacements.data(),
            MPI_BYTE,
            received.Data(),
            receive_bytes.data(),
            receive_displacements.data(),
            MPI_BYTE,
            context_.Communicator()) != MPI_SUCCESS) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    return BLITZAR_STATUS_OK;
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
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

#if defined(BLITZAR_HAS_MPI)
    const int size = context_.Size();
    std::vector<int> counts;
    try {
        counts.assign(static_cast<std::size_t>(size), 0);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    if (!ToMpiCount(local_packets.Size(), counts[static_cast<std::size_t>(context_.Rank())])) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    int local_count = counts[static_cast<std::size_t>(context_.Rank())];
    if (MPI_Allgather(
            &local_count,
            1,
            MPI_INT,
            counts.data(),
            1,
            MPI_INT,
            context_.Communicator()) != MPI_SUCCESS) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    std::vector<int> bytes;
    std::vector<int> displacements;
    try {
        bytes.assign(static_cast<std::size_t>(size), 0);
        displacements.assign(static_cast<std::size_t>(size), 0);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    std::size_t total = 0;
    for (int peer = 0; peer < size; ++peer) {
        if (counts[static_cast<std::size_t>(peer)] < 0 ||
            !ToByteCount(
                static_cast<std::size_t>(counts[static_cast<std::size_t>(peer)]),
                bytes[static_cast<std::size_t>(peer)]) ||
            !ToByteCount(total, displacements[static_cast<std::size_t>(peer)])) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        const std::size_t count = static_cast<std::size_t>(
            counts[static_cast<std::size_t>(peer)]);
        if (total > std::numeric_limits<std::size_t>::max() - count) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        total += count;
    }
    try {
        gathered.Resize(total);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    std::vector<int> receive_displacements;
    try {
        receive_displacements = displacements;
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    if (MPI_Allgatherv(
            local_packets.Data(),
            bytes[static_cast<std::size_t>(context_.Rank())],
            MPI_BYTE,
            gathered.Data(),
            bytes.data(),
            receive_displacements.data(),
            MPI_BYTE,
            context_.Communicator()) != MPI_SUCCESS) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    return BLITZAR_STATUS_OK;
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

}  // namespace blitzar_parallel
