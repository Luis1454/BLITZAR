#include "parallel/MpiGhostTransport.hpp"

#include "parallel/MpiGhostProtocol.hpp"
#include "parallel/MpiGhostState.hpp"
#include "parallel/MpiPacketTransport.hpp"
#include "parallel/MpiSessionNative.hpp"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <limits>
#include <new>
#include <utility>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

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
            MpiGhostProtocol::CancelRequests(state.receive_requests);
            MpiGhostProtocol::CancelRequests(state.send_requests);
        }
        else {
            state.receive_requests.clear();
            state.send_requests.clear();
        }
    }
#endif

    ClearExchange(state);

    state.transfer = {};
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

MpiGhostExchange::TransferStats MpiGhostExchange::Transfer() const noexcept
{
    return impl_ == nullptr ? TransferStats{} : impl_->transfer;
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
        state.transfer = {};

        const std::size_t peer_count = static_cast<std::size_t>(session_.Size());
        const std::size_t remote_peer_count = peer_count > 0 ? peer_count - 1 : 0;
        const std::size_t chunks = MpiGhostProtocol::ChunkCount(send_capacity);

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
        if (!MpiGhostProtocol::ToWireSize(send_capacity, local_wire_capacity) ||
            !MpiGhostProtocol::ToWireSize(receive_capacity, receive_wire_capacity)) {
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

        if (!MpiGhostProtocol::EnsureCapacity(state.local_wire, local_wire_capacity) ||
            !MpiGhostProtocol::EnsureCapacity(state.receive_wire, receive_wire_capacity)) {
            return BLITZAR_STATUS_ALLOCATION_FAILURE;
        }

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
