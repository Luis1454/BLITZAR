#include "parallel/MpiGhostTransport.hpp"

#include "parallel/MpiPacketTransport.hpp"
#include "parallel/MpiSessionNative.hpp"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <limits>
#include <new>
#include <utility>
#include <vector>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

struct MpiGhostExchange::Impl final {
    struct ReceiveChunk final {
        std::size_t peer_index{};
        std::size_t packet_offset{};
    };

    bool active{false};
    std::size_t send_capacity{0};
    std::size_t receive_capacity{0};
#if defined(BLITZAR_HAS_MPI)
    std::vector<std::byte> local_wire;
    std::vector<std::byte> receive_wire;
    std::vector<int> peer_counts;
    std::vector<std::size_t> peer_capacities;
    std::vector<std::size_t> wire_offsets;
    std::vector<std::size_t> receive_counts;
    std::vector<std::size_t> offsets;
    std::vector<MPI_Request> receive_requests;
    std::vector<MPI_Request> send_requests;
    std::vector<MPI_Status> receive_statuses;
    std::vector<ReceiveChunk> receive_chunks;
#endif
};

namespace {

[[maybe_unused]] constexpr int GhostDataTag = 7102;

#if defined(BLITZAR_HAS_MPI)

[[nodiscard]] bool ToWireBytes(std::size_t packets, int& bytes) noexcept
{
    if (packets > static_cast<std::size_t>(INT_MAX) / ParticleWireBytes) {
        return false;
    }

    bytes = static_cast<int>(packets * ParticleWireBytes);

    return true;
}

[[nodiscard]] bool ToWireSize(std::size_t packets, std::size_t& bytes) noexcept
{
    if (packets > std::numeric_limits<std::size_t>::max() / ParticleWireBytes) {
        return false;
    }

    bytes = packets * ParticleWireBytes;

    return true;
}

[[nodiscard]] std::size_t PointChunkPackets() noexcept
{
    return static_cast<std::size_t>(INT_MAX) / ParticleWireBytes;
}

[[nodiscard]] std::size_t ChunkCount(std::size_t packets) noexcept
{
    const std::size_t chunk_packets = PointChunkPackets();

    return packets == 0 ? 0 : 1 + (packets - 1) / chunk_packets;
}

template <typename Value>
[[nodiscard]] bool EnsureCapacity(std::vector<Value>& values, std::size_t capacity) noexcept
{
    if (capacity <= values.capacity()) {
        return true;
    }

    try {
        values.reserve(capacity);
    }
    catch (const std::length_error&) {
        return false;
    }
    catch (const std::bad_alloc&) {
        return false;
    }

    return true;
}

template <typename Value>
[[nodiscard]] bool ResizeWithinCapacity(std::vector<Value>& values, std::size_t size) noexcept
{
    if (size > values.capacity()) {
        return false;
    }

    values.resize(size);

    return true;
}

[[nodiscard]] blitzar_status WaitRequests(std::vector<MPI_Request>& requests) noexcept
{
    if (requests.empty()) {
        return BLITZAR_STATUS_OK;
    }
    if (requests.size() > static_cast<std::size_t>(INT_MAX)) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return MPI_Waitall(static_cast<int>(requests.size()), requests.data(), MPI_STATUSES_IGNORE) ==
                   MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}

[[nodiscard]] blitzar_status WaitRequests(std::vector<MPI_Request>& requests,
    std::vector<MPI_Status>& statuses) noexcept
{
    if (requests.empty()) {
        return BLITZAR_STATUS_OK;
    }
    if (requests.size() > static_cast<std::size_t>(INT_MAX) || statuses.size() != requests.size()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return MPI_Waitall(static_cast<int>(requests.size()), requests.data(), statuses.data()) ==
                   MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}

void CancelRequests(std::vector<MPI_Request>& requests) noexcept
{
    for (MPI_Request& request : requests) {
        if (request != MPI_REQUEST_NULL) {
            (void)MPI_Cancel(&request);
        }
    }

    (void)WaitRequests(requests);

    requests.clear();
}

#endif

} // namespace

void MpiGhostTransport::ClearExchange(MpiGhostExchange::Impl& state) noexcept
{
    state.active = false;
#if defined(BLITZAR_HAS_MPI)

    state.local_wire.clear();
    state.receive_wire.clear();
    state.receive_requests.clear();
    state.send_requests.clear();
    state.receive_statuses.clear();
    state.receive_chunks.clear();
#endif
}

void MpiGhostTransport::AbortExchange(MpiGhostExchange::Impl& state) noexcept
{
#if defined(BLITZAR_HAS_MPI)
    if (state.active) {
        int initialized = 0;
        int finalized = 0;

        if (MPI_Initialized(&initialized) == MPI_SUCCESS && initialized != 0 &&
            MPI_Finalized(&finalized) == MPI_SUCCESS && finalized == 0) {
            CancelRequests(state.receive_requests);
            CancelRequests(state.send_requests);
        }
        else {
            state.receive_requests.clear();
            state.send_requests.clear();
        }
    }
#endif

    ClearExchange(state);
}

MpiGhostExchange::MpiGhostExchange() noexcept = default;

MpiGhostExchange::~MpiGhostExchange() noexcept
{
    if (impl_ != nullptr) {
        MpiGhostTransport::AbortExchange(*impl_);
    }
}

MpiGhostExchange::MpiGhostExchange(MpiGhostExchange&& other) noexcept = default;

MpiGhostExchange& MpiGhostExchange::operator=(MpiGhostExchange&& other) noexcept
{
    if (this == &other) {
        return *this;
    }

    if (impl_ != nullptr) {
        MpiGhostTransport::AbortExchange(*impl_);
    }

    impl_ = std::move(other.impl_);

    return *this;
}

MpiGhostTransport::MpiGhostTransport(const MpiSession& session,
    const MpiCollectives& collectives, const MpiPacketTransport& packets) noexcept
    : session_(session), collectives_(collectives), packets_(packets)
{
}

blitzar_status MpiGhostTransport::Prepare(MpiGhostExchange& exchange,
    std::size_t send_capacity, std::size_t receive_capacity) const noexcept
{
    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        exchange.impl_.reset();

        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    if (exchange.impl_ != nullptr && exchange.impl_->active) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        if (exchange.impl_ == nullptr) {
            exchange.impl_ = std::make_unique<MpiGhostExchange::Impl>();
        }

        MpiGhostExchange::Impl& state = *exchange.impl_;

        const std::size_t peer_count = static_cast<std::size_t>(session_.Size());
        const std::size_t remote_peer_count = peer_count > 0 ? peer_count - 1 : 0;
        const std::size_t chunks = ChunkCount(send_capacity);

        if (remote_peer_count != 0 &&
            chunks > std::numeric_limits<std::size_t>::max() / remote_peer_count) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const std::size_t request_capacity = chunks * remote_peer_count;
        std::size_t local_wire_capacity = 0;
        std::size_t receive_wire_capacity = 0;

        if (request_capacity > static_cast<std::size_t>(INT_MAX)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        if (!ToWireSize(send_capacity, local_wire_capacity) ||
            !ToWireSize(receive_capacity, receive_wire_capacity)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        state.send_capacity = send_capacity;
        state.receive_capacity = receive_capacity;

        state.peer_counts.resize(peer_count);
        state.peer_capacities.resize(peer_count);
        state.wire_offsets.resize(peer_count);
        state.receive_counts.resize(peer_count);
        state.offsets.resize(peer_count);
        state.receive_requests.reserve(request_capacity);
        state.send_requests.reserve(request_capacity);
        state.receive_statuses.reserve(request_capacity);
        state.receive_chunks.reserve(request_capacity);

        if (!EnsureCapacity(state.local_wire, local_wire_capacity)) {
            return BLITZAR_STATUS_ALLOCATION_FAILURE;
        }

        (void)receive_wire_capacity;

        state.local_wire.clear();
        state.receive_wire.clear();
        state.receive_requests.clear();
        state.send_requests.clear();
        state.receive_statuses.clear();
        state.receive_chunks.clear();
        std::fill(state.receive_counts.begin(), state.receive_counts.end(), 0);
        std::fill(state.offsets.begin(), state.offsets.end(), 0);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
#else

    (void)exchange;
    (void)send_capacity;
    (void)receive_capacity;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

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

    MpiGhostExchange::Impl* state_pointer = exchange.impl_.get();

    blitzar_status preparation_status =
        state_pointer == nullptr ? BLITZAR_STATUS_INVALID_ARGUMENT : BLITZAR_STATUS_OK;

    if (state_pointer != nullptr && preparation_status == BLITZAR_STATUS_OK) {
        const MpiGhostExchange::Impl& state = *state_pointer;
        const std::size_t peer_count = static_cast<std::size_t>(session_.Size());

        if (state.active || local.size() > static_cast<std::size_t>(INT_MAX) ||
            state.peer_counts.size() != peer_count ||
            state.peer_capacities.size() != peer_count ||
            state.wire_offsets.size() != peer_count ||
            state.receive_counts.size() != peer_count || state.offsets.size() != peer_count) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    std::size_t local_bytes = 0;

    if (preparation_status == BLITZAR_STATUS_OK && !ToWireSize(local.size(), local_bytes)) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (preparation_status == BLITZAR_STATUS_OK) {
        MpiGhostExchange::Impl& state = *state_pointer;

        if (!EnsureCapacity(state.local_wire, local_bytes) ||
            !ResizeWithinCapacity(state.local_wire, local_bytes)) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        else {
            if (!ParticleWireCodec::Encode(local,
                    std::span<std::byte>(state.local_wire.data(), state.local_wire.size()))) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
            }
        }
    }

    blitzar_status global_preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = collectives_.SynchronizeStatus(
        preparation_status, "MpiGhostTransport", "ghost-begin-prepare", global_preparation_status);

    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_preparation_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status
                                                           : global_preparation_status;
    }

    MpiGhostExchange::Impl& state = *state_pointer;

    const blitzar_status peer_count_status =
        packets_.AllGatherCounts(static_cast<int>(local.size()), state.peer_counts);

    blitzar_status global_peer_count_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status peer_count_synchronization_status = collectives_.SynchronizeStatus(
        peer_count_status, "MpiGhostTransport", "ghost-capacity", global_peer_count_status);

    if (peer_count_synchronization_status != BLITZAR_STATUS_OK ||
        global_peer_count_status != BLITZAR_STATUS_OK) {
        return peer_count_synchronization_status != BLITZAR_STATUS_OK
                   ? peer_count_synchronization_status
                   : global_peer_count_status;
    }

    const std::size_t peer_count = static_cast<std::size_t>(session_.Size());
    const std::size_t remote_peer_count = peer_count - 1;
    const std::size_t chunk_packets = PointChunkPackets();
    std::size_t receive_slots = 0;
    std::size_t receive_request_count = 0;
    std::size_t send_request_count = 0;
    std::size_t receive_wire_size = 0;

    preparation_status = BLITZAR_STATUS_OK;

    std::fill(state.peer_capacities.begin(), state.peer_capacities.end(), 0);
    std::fill(state.wire_offsets.begin(), state.wire_offsets.end(), 0);

    if (chunk_packets == 0) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t peer = 0; preparation_status == BLITZAR_STATUS_OK && peer < peer_count;
         ++peer) {
        const int peer_count_value = state.peer_counts[peer];

        if (peer_count_value < 0) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;

            break;
        }

        state.peer_capacities[peer] = static_cast<std::size_t>(peer_count_value);

        if (peer == static_cast<std::size_t>(session_.Rank())) {
            continue;
        }

        state.wire_offsets[peer] = receive_slots;

        if (receive_slots > std::numeric_limits<std::size_t>::max() -
                                state.peer_capacities[peer] ||
            receive_request_count >
                std::numeric_limits<std::size_t>::max() -
                    ChunkCount(state.peer_capacities[peer])) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;

            break;
        }

        receive_slots += state.peer_capacities[peer];
        receive_request_count += ChunkCount(state.peer_capacities[peer]);
    }

    if (preparation_status == BLITZAR_STATUS_OK &&
        remote_peer_count != 0 &&
            ChunkCount(local.size()) >
                std::numeric_limits<std::size_t>::max() / remote_peer_count) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    else if (preparation_status == BLITZAR_STATUS_OK) {
        send_request_count = ChunkCount(local.size()) * remote_peer_count;
    }

    if (preparation_status == BLITZAR_STATUS_OK &&
        (receive_request_count > static_cast<std::size_t>(INT_MAX) ||
            send_request_count > static_cast<std::size_t>(INT_MAX) ||
            !ToWireSize(receive_slots, receive_wire_size))) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (preparation_status == BLITZAR_STATUS_OK &&
        (!EnsureCapacity(state.receive_wire, receive_wire_size) ||
            !ResizeWithinCapacity(state.receive_wire, receive_wire_size) ||
            !EnsureCapacity(state.receive_requests, receive_request_count) ||
            !EnsureCapacity(state.send_requests, send_request_count) ||
            !EnsureCapacity(state.receive_statuses, receive_request_count) ||
            !EnsureCapacity(state.receive_chunks, receive_request_count) ||
            !ResizeWithinCapacity(state.receive_requests, receive_request_count) ||
            !ResizeWithinCapacity(state.send_requests, send_request_count) ||
            !ResizeWithinCapacity(state.receive_statuses, receive_request_count) ||
            !ResizeWithinCapacity(state.receive_chunks, receive_request_count))) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (preparation_status == BLITZAR_STATUS_OK) {
        std::fill(state.receive_counts.begin(), state.receive_counts.end(), 0);
        std::fill(state.offsets.begin(), state.offsets.end(), 0);
    }

    blitzar_status global_capacity_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status capacity_synchronization_status = collectives_.SynchronizeStatus(
        preparation_status, "MpiGhostTransport", "ghost-capacity-prepare", global_capacity_status);

    if (capacity_synchronization_status != BLITZAR_STATUS_OK ||
        global_capacity_status != BLITZAR_STATUS_OK) {
        return capacity_synchronization_status != BLITZAR_STATUS_OK
                   ? capacity_synchronization_status
                   : global_capacity_status;
    }

    state.active = true;

    std::size_t receive_request_index = 0;
    std::size_t send_request_index = 0;

    for (int peer = 0; peer < session_.Size(); ++peer) {
        if (peer == session_.Rank()) {
            continue;
        }

        const std::size_t peer_index = static_cast<std::size_t>(peer);

        std::size_t packet_offset = 0;

        while (packet_offset < state.peer_capacities[peer_index]) {
            const std::size_t chunk = std::min(
                state.peer_capacities[peer_index] - packet_offset, PointChunkPackets());

            int bytes = 0;

            if (!ToWireBytes(chunk, bytes)) {
                AbortExchange(state);

                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            if (receive_request_index >= state.receive_chunks.size() ||
                receive_request_index >= state.receive_requests.size()) {
                AbortExchange(state);

                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            state.receive_chunks[receive_request_index] = {peer_index, packet_offset};
            state.receive_requests[receive_request_index] = MPI_REQUEST_NULL;

            if (MPI_Irecv(state.receive_wire.data() +
                              (state.wire_offsets[peer_index] + packet_offset) * ParticleWireBytes,
                    bytes, MPI_BYTE, peer, GhostDataTag, session_.Native().communicator,
                    &state.receive_requests[receive_request_index]) != MPI_SUCCESS) {
                AbortExchange(state);

                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            ++receive_request_index;

            packet_offset += chunk;
        }

        packet_offset = 0;

        while (packet_offset < local.size()) {
            const std::size_t chunk = std::min(local.size() - packet_offset, PointChunkPackets());

            int local_bytes_count = 0;

            if (!ToWireBytes(chunk, local_bytes_count)) {
                AbortExchange(state);

                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            if (send_request_index >= state.send_requests.size()) {
                AbortExchange(state);

                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            state.send_requests[send_request_index] = MPI_REQUEST_NULL;

            const std::byte* local_data = state.local_wire.data() +
                                          packet_offset * ParticleWireBytes;

            if (MPI_Isend(local_data, local_bytes_count, MPI_BYTE, peer, GhostDataTag,
                    session_.Native().communicator, &state.send_requests[send_request_index]) !=
                MPI_SUCCESS) {
                AbortExchange(state);

                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            ++send_request_index;

            packet_offset += chunk;
        }
    }

    if (receive_request_index != receive_request_count || send_request_index != send_request_count) {
        AbortExchange(state);

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return BLITZAR_STATUS_OK;
#else

    (void)local;
    (void)exchange;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiGhostTransport::Complete(
    MpiGhostExchange& exchange, PacketBuffer& ghosts) const noexcept
{
    ghosts.Clear();

    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)

    const bool exchange_active = exchange.impl_ != nullptr && exchange.impl_->active;
    blitzar_status global_active_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status active_synchronization_status = collectives_.SynchronizeStatus(
        exchange_active ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT, "MpiGhostTransport",
        "ghost-complete-preflight", global_active_status);

    if (active_synchronization_status != BLITZAR_STATUS_OK ||
        global_active_status != BLITZAR_STATUS_OK) {
        Abort(exchange);

        return active_synchronization_status != BLITZAR_STATUS_OK ? active_synchronization_status
                                                                  : global_active_status;
    }
    if (exchange.impl_ == nullptr || !exchange.impl_->active) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    MpiGhostExchange::Impl& state = *exchange.impl_;

    blitzar_status status = WaitRequests(state.receive_requests, state.receive_statuses);

    if (status != BLITZAR_STATUS_OK) {
        AbortExchange(state);

        return status;
    }

    status = WaitRequests(state.send_requests);

    if (status != BLITZAR_STATUS_OK) {
        AbortExchange(state);

        return status;
    }

    std::size_t total = 0;
    blitzar_status count_status = BLITZAR_STATUS_OK;

    std::fill(state.receive_counts.begin(), state.receive_counts.end(), 0);

    for (std::size_t request = 0; request < state.receive_chunks.size(); ++request) {
        int bytes = 0;

        if (MPI_Get_count(&state.receive_statuses[request], MPI_BYTE, &bytes) != MPI_SUCCESS ||
            bytes < 0 || bytes % static_cast<int>(ParticleWireBytes) != 0) {
            count_status = BLITZAR_STATUS_INTERNAL_ERROR;

            break;
        }

        const MpiGhostExchange::Impl::ReceiveChunk chunk = state.receive_chunks[request];
        const std::size_t packet_count =
            static_cast<std::size_t>(bytes) / ParticleWireBytes;

        std::size_t& peer_count = state.receive_counts[chunk.peer_index];

        if (packet_count > state.peer_capacities[chunk.peer_index] ||
            peer_count > state.peer_capacities[chunk.peer_index] - packet_count) {
                count_status = BLITZAR_STATUS_INTERNAL_ERROR;

            break;
        }

        peer_count += packet_count;
    }

    if (count_status == BLITZAR_STATUS_OK) {
        for (int peer = 0; peer < session_.Size(); ++peer) {
            const std::size_t peer_index = static_cast<std::size_t>(peer);
            const std::size_t count = state.receive_counts[peer_index];

            if (total > std::numeric_limits<std::size_t>::max() - count) {
                count_status = BLITZAR_STATUS_INTERNAL_ERROR;

                break;
            }

            state.offsets[peer_index] = total;
            total += count;
        }
    }

    blitzar_status global_count_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status count_synchronization_status = collectives_.SynchronizeStatus(
        count_status, "MpiGhostTransport", "ghost-count-prepare", global_count_status);

    if (count_synchronization_status != BLITZAR_STATUS_OK ||
        global_count_status != BLITZAR_STATUS_OK) {
        AbortExchange(state);

        return count_synchronization_status != BLITZAR_STATUS_OK ? count_synchronization_status
                                                                 : global_count_status;
    }

    blitzar_status preparation_status = BLITZAR_STATUS_OK;

    if (preparation_status == BLITZAR_STATUS_OK) {
        if (!ghosts.EnsureCapacity(total) || !ghosts.ResizeBounded(total)) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    blitzar_status global_preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = collectives_.SynchronizeStatus(
        preparation_status, "MpiGhostTransport", "ghost-data-prepare", global_preparation_status);

    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_preparation_status != BLITZAR_STATUS_OK) {
        AbortExchange(state);

        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status
                                                           : global_preparation_status;
    }

    blitzar_status decode_status = BLITZAR_STATUS_OK;

    for (std::size_t request = 0; request < state.receive_chunks.size(); ++request) {
        int bytes = 0;

        if (MPI_Get_count(&state.receive_statuses[request], MPI_BYTE, &bytes) != MPI_SUCCESS ||
            bytes < 0 || bytes % static_cast<int>(ParticleWireBytes) != 0) {
            decode_status = BLITZAR_STATUS_INVALID_ARGUMENT;

            break;
        }

        const MpiGhostExchange::Impl::ReceiveChunk chunk = state.receive_chunks[request];
        const std::size_t packet_count =
            static_cast<std::size_t>(bytes) / ParticleWireBytes;

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
            decode_status = BLITZAR_STATUS_INVALID_ARGUMENT;

            break;
        }
    }

    blitzar_status global_decode_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status decode_synchronization_status = collectives_.SynchronizeStatus(
        decode_status, "MpiGhostTransport", "ghost-data-decode", global_decode_status);

    ClearExchange(state);

    return decode_synchronization_status != BLITZAR_STATUS_OK ? decode_synchronization_status
                                                              : global_decode_status;
#else

    (void)exchange;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

bool MpiGhostTransport::IsActive(const MpiGhostExchange& exchange) const noexcept
{
#if defined(BLITZAR_HAS_MPI)
    return exchange.impl_ != nullptr && exchange.impl_->active;
#else

    (void)exchange;

    return false;
#endif
}

void MpiGhostTransport::Abort(MpiGhostExchange& exchange) const noexcept
{
    if (exchange.impl_ == nullptr) {
        return;
    }
#if defined(BLITZAR_HAS_MPI)

    AbortExchange(*exchange.impl_);
#else
    exchange.impl_.reset();
#endif
}

} // namespace blitzar_parallel
