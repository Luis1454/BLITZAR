#include "parallel/MpiGhostTransport.hpp"

#include "parallel/MpiSessionNative.hpp"

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

struct MpiGhostExchange::Impl final {
    bool active{false};
#if defined(BLITZAR_HAS_MPI)
    std::vector<std::byte> local_wire;
    std::vector<std::byte> receive_wire;
    int local_count{0};
    std::vector<int> receive_counts;
    std::vector<std::size_t> offsets;
    std::vector<MPI_Request> requests;
#endif
};

namespace {

constexpr int GhostCountTag = 7101;
constexpr int GhostDataTag = 7102;

#if defined(BLITZAR_HAS_MPI)

[[nodiscard]] bool ToWireBytes(std::size_t packets, int& bytes) noexcept
{
    if (packets > static_cast<std::size_t>(INT_MAX) / ParticleWireBytes) {
        return false;
    }
    bytes = static_cast<int>(packets * ParticleWireBytes);
    return true;
}

[[nodiscard]] bool ToWireSize(
    std::size_t packets, std::size_t& bytes) noexcept
{
    if (packets > std::numeric_limits<std::size_t>::max() /
                          ParticleWireBytes) {
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

[[nodiscard]] blitzar_status WaitRequests(
    std::vector<MPI_Request>& requests) noexcept
{
    if (requests.empty()) {
        return BLITZAR_STATUS_OK;
    }
    if (requests.size() > static_cast<std::size_t>(INT_MAX)) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    return MPI_Waitall(
               static_cast<int>(requests.size()),
               requests.data(),
               MPI_STATUSES_IGNORE) == MPI_SUCCESS
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

}  // namespace

void MpiGhostTransport::ClearExchange(
    MpiGhostExchange::Impl& state) noexcept
{
    state.active = false;
#if defined(BLITZAR_HAS_MPI)
    state.local_wire.clear();
    state.receive_wire.clear();
    state.receive_counts.clear();
    state.offsets.clear();
    state.requests.clear();
#endif
}

void MpiGhostTransport::AbortExchange(
    MpiGhostExchange::Impl& state) noexcept
{
#if defined(BLITZAR_HAS_MPI)
    if (state.active) {
        int initialized = 0;
        int finalized = 0;
        if (MPI_Initialized(&initialized) == MPI_SUCCESS &&
            initialized != 0 && MPI_Finalized(&finalized) == MPI_SUCCESS &&
            finalized == 0) {
            CancelRequests(state.requests);
        } else {
            state.requests.clear();
        }
    }
#endif
    ClearExchange(state);
}

MpiGhostExchange::MpiGhostExchange() noexcept = default;

MpiGhostExchange::~MpiGhostExchange() noexcept
{
#if defined(BLITZAR_HAS_MPI)
    if (impl_ != nullptr && impl_->active) {
        MpiGhostTransport::AbortExchange(*impl_);
    }
#endif
}

MpiGhostExchange::MpiGhostExchange(MpiGhostExchange&& other) noexcept = default;

MpiGhostExchange& MpiGhostExchange::operator=(
    MpiGhostExchange&& other) noexcept = default;

MpiGhostTransport::MpiGhostTransport(
    const MpiSession& session,
    const MpiCollectives& collectives) noexcept
    : session_(session), collectives_(collectives)
{
}

blitzar_status MpiGhostTransport::Begin(
    std::span<const ParticlePacket> local,
    MpiGhostExchange& exchange) const noexcept
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
        try {
            exchange.impl_ = std::make_unique<MpiGhostExchange::Impl>();
        } catch (const std::bad_alloc&) {
            blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
            const blitzar_status synchronization_status =
                collectives_.SynchronizeStatus(
                    BLITZAR_STATUS_ALLOCATION_FAILURE,
                    "MpiGhostTransport",
                    "ghost-begin-prepare",
                    global_status);
            return synchronization_status != BLITZAR_STATUS_OK
                       ? synchronization_status
                       : global_status;
        }
    }
    MpiGhostExchange::Impl& state = *exchange.impl_;
    blitzar_status preparation_status =
        state.active || local.size() > static_cast<std::size_t>(INT_MAX)
            ? BLITZAR_STATUS_INVALID_ARGUMENT
            : BLITZAR_STATUS_OK;
    std::size_t local_bytes = 0;
    try {
        if (preparation_status == BLITZAR_STATUS_OK &&
            !ToWireSize(local.size(), local_bytes)) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        if (preparation_status == BLITZAR_STATUS_OK) {
            state.local_wire.resize(local_bytes);
            if (!ParticleWireCodec::Encode(
                    local,
                    std::span<std::byte>(
                        state.local_wire.data(), state.local_wire.size()))) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
            }
            state.receive_counts.assign(
                static_cast<std::size_t>(session_.Size()), 0);
            state.offsets.assign(static_cast<std::size_t>(session_.Size()), 0);
            state.requests.clear();
            state.requests.reserve(
                static_cast<std::size_t>(session_.Size() - 1) * 2);
        }
    } catch (const std::length_error&) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    blitzar_status global_preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        collectives_.SynchronizeStatus(
            preparation_status,
            "MpiGhostTransport",
            "ghost-begin-prepare",
            global_preparation_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_preparation_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_preparation_status;
    }
    state.local_count = static_cast<int>(local.size());
    state.active = true;
    for (int peer = 0; peer < session_.Size(); ++peer) {
        if (peer == session_.Rank()) {
            continue;
        }
        state.requests.push_back(MPI_REQUEST_NULL);
        if (MPI_Irecv(
                &state.receive_counts[static_cast<std::size_t>(peer)],
                1,
                MPI_INT,
                peer,
                GhostCountTag,
                session_.Native().communicator,
                &state.requests.back()) != MPI_SUCCESS) {
            AbortExchange(state);
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        state.requests.push_back(MPI_REQUEST_NULL);
        if (MPI_Isend(
                &state.local_count,
                1,
                MPI_INT,
                peer,
                GhostCountTag,
                session_.Native().communicator,
                &state.requests.back()) != MPI_SUCCESS) {
            AbortExchange(state);
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
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
    const bool exchange_active =
        exchange.impl_ != nullptr && exchange.impl_->active;
    blitzar_status global_active_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status active_synchronization_status =
        collectives_.SynchronizeStatus(
            exchange_active ? BLITZAR_STATUS_OK
                            : BLITZAR_STATUS_INVALID_ARGUMENT,
            "MpiGhostTransport",
            "ghost-complete-preflight",
            global_active_status);
    if (active_synchronization_status != BLITZAR_STATUS_OK ||
        global_active_status != BLITZAR_STATUS_OK) {
        Abort(exchange);
        return active_synchronization_status != BLITZAR_STATUS_OK
                   ? active_synchronization_status
                   : global_active_status;
    }
    if (exchange.impl_ == nullptr || !exchange.impl_->active) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    MpiGhostExchange::Impl& state = *exchange.impl_;
    blitzar_status status = WaitRequests(state.requests);
    if (status != BLITZAR_STATUS_OK) {
        AbortExchange(state);
        return status;
    }

    std::size_t total = 0;
    blitzar_status count_status = BLITZAR_STATUS_OK;
    for (int peer = 0; peer < session_.Size(); ++peer) {
        const int count = state.receive_counts[static_cast<std::size_t>(peer)];
        if (count < 0 || total > std::numeric_limits<std::size_t>::max() -
                                  static_cast<std::size_t>(count)) {
            count_status = BLITZAR_STATUS_INTERNAL_ERROR;
            break;
        }
        state.offsets[static_cast<std::size_t>(peer)] = total;
        total += static_cast<std::size_t>(count);
    }
    blitzar_status global_count_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status count_synchronization_status =
        collectives_.SynchronizeStatus(
            count_status,
            "MpiGhostTransport",
            "ghost-count-prepare",
            global_count_status);
    if (count_synchronization_status != BLITZAR_STATUS_OK ||
        global_count_status != BLITZAR_STATUS_OK) {
        AbortExchange(state);
        return count_synchronization_status != BLITZAR_STATUS_OK
                   ? count_synchronization_status
                   : global_count_status;
    }

    const std::size_t chunk_packets = PointChunkPackets();
    std::size_t request_count = 0;
    blitzar_status preparation_status = BLITZAR_STATUS_OK;
    std::size_t total_wire_size = 0;
    if (chunk_packets == 0 || !ToWireSize(total, total_wire_size)) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (preparation_status == BLITZAR_STATUS_OK) {
        request_count = 0;
        for (int peer = 0; peer < session_.Size(); ++peer) {
            if (peer == session_.Rank()) {
                continue;
            }
            const std::size_t receive_count = static_cast<std::size_t>(
                state.receive_counts[static_cast<std::size_t>(peer)]);
            const std::size_t peer_requests =
                ChunkCount(receive_count) + ChunkCount(state.local_wire.size() /
                                                        ParticleWireBytes);
            if (request_count > std::numeric_limits<std::size_t>::max() -
                                    peer_requests) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                break;
            }
            request_count += peer_requests;
        }
        if (request_count > static_cast<std::size_t>(INT_MAX)) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    std::size_t receive_wire_size = 0;
    if (preparation_status == BLITZAR_STATUS_OK &&
        !ToWireSize(total, receive_wire_size)) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    try {
        if (preparation_status == BLITZAR_STATUS_OK) {
            ghosts.Resize(total);
            state.receive_wire.resize(receive_wire_size);
            state.requests.clear();
            state.requests.reserve(request_count);
        }
    } catch (const std::length_error&) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    blitzar_status global_preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        collectives_.SynchronizeStatus(
            preparation_status,
            "MpiGhostTransport",
            "ghost-data-prepare",
            global_preparation_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_preparation_status != BLITZAR_STATUS_OK) {
        AbortExchange(state);
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_preparation_status;
    }

    for (int peer = 0; peer < session_.Size(); ++peer) {
        if (peer == session_.Rank()) {
            continue;
        }
        const std::size_t peer_index = static_cast<std::size_t>(peer);
        const std::size_t receive_count =
            static_cast<std::size_t>(state.receive_counts[peer_index]);
        for (std::size_t packet = 0; packet < receive_count;
             packet += chunk_packets) {
            const std::size_t chunk =
                std::min(receive_count - packet, chunk_packets);
            int bytes = 0;
            if (!ToWireBytes(chunk, bytes)) {
                AbortExchange(state);
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
            state.requests.push_back(MPI_REQUEST_NULL);
            if (MPI_Irecv(
                    state.receive_wire.data() +
                        (state.offsets[peer_index] + packet) * ParticleWireBytes,
                    bytes,
                    MPI_BYTE,
                    peer,
                    GhostDataTag,
                    session_.Native().communicator,
                    &state.requests.back()) != MPI_SUCCESS) {
                AbortExchange(state);
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }
        }
        const std::size_t local_count =
            static_cast<std::size_t>(state.local_count);
        for (std::size_t packet = 0; packet < local_count;
             packet += chunk_packets) {
            const std::size_t chunk =
                std::min(local_count - packet, chunk_packets);
            int bytes = 0;
            if (!ToWireBytes(chunk, bytes)) {
                AbortExchange(state);
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
            state.requests.push_back(MPI_REQUEST_NULL);
            if (MPI_Isend(
                    state.local_wire.data() + packet * ParticleWireBytes,
                    bytes,
                    MPI_BYTE,
                    peer,
                    GhostDataTag,
                    session_.Native().communicator,
                    &state.requests.back()) != MPI_SUCCESS) {
                AbortExchange(state);
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }
        }
    }
    status = WaitRequests(state.requests);
    if (status != BLITZAR_STATUS_OK) {
        AbortExchange(state);
        return status;
    }

    blitzar_status decode_status = BLITZAR_STATUS_OK;
    for (int peer = 0; peer < session_.Size(); ++peer) {
        const std::size_t peer_index = static_cast<std::size_t>(peer);
        const std::size_t count = static_cast<std::size_t>(
            state.receive_counts[peer_index]);
        if (!ParticleWireCodec::Decode(
                std::span<const std::byte>(
                    state.receive_wire.data(), state.receive_wire.size())
                    .subspan(
                        state.offsets[peer_index] * ParticleWireBytes,
                        count * ParticleWireBytes),
                ghosts.View().subspan(state.offsets[peer_index], count))) {
            decode_status = BLITZAR_STATUS_INVALID_ARGUMENT;
            break;
        }
    }
    blitzar_status global_decode_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status decode_synchronization_status =
        collectives_.SynchronizeStatus(
            decode_status,
            "MpiGhostTransport",
            "ghost-data-decode",
            global_decode_status);
    ClearExchange(state);
    return decode_synchronization_status != BLITZAR_STATUS_OK
               ? decode_synchronization_status
               : global_decode_status;
#else
    (void)exchange;
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

bool MpiGhostTransport::IsActive(
    const MpiGhostExchange& exchange) const noexcept
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

}  // namespace blitzar_parallel
