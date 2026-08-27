#include "mpi/native/MpiNative.hpp"
#include "mpi/native/MpiNativeState.hpp"

#include <climits>
#include <new>
#include <stdexcept>

#if defined(BLITZAR_HAS_MPI)

namespace blitzar_parallel {

namespace {

[[nodiscard]] bool RuntimeLive() noexcept
{
    int initialized = 0;
    int finalized = 0;

    return MPI_Initialized(&initialized) == MPI_SUCCESS && initialized != 0 &&
           MPI_Finalized(&finalized) == MPI_SUCCESS && finalized == 0;
}

template <typename Request> void CancelRequests(std::vector<Request>& requests) noexcept
{
    for (Request& request : requests) {
        if (request != MPI_REQUEST_NULL) {
            (void)MPI_Cancel(&request);
        }
    }

    if (!requests.empty() && requests.size() <= static_cast<std::size_t>(INT_MAX)) {
        (void)MPI_Waitall(static_cast<int>(requests.size()), requests.data(), MPI_STATUSES_IGNORE);
    }
}

} // namespace

#else

namespace blitzar_parallel {

#endif

MpiNativeGhost::MpiNativeGhost() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        impl_.reset();
    }
}

MpiNativeGhost::~MpiNativeGhost() noexcept
{
    Cancel();
}

void MpiNativeGhost::Reset() noexcept
{
    if (impl_ == nullptr) {
        return;
    }

#if defined(BLITZAR_HAS_MPI)

    impl_->receive_requests.clear();
    impl_->send_requests.clear();
    impl_->receive_statuses.clear();

    impl_->receive_posted = 0;
    impl_->send_posted = 0;
#endif
}

void MpiNativeGhost::Cancel() noexcept
{
    if (impl_ == nullptr) {
        return;
    }

#if defined(BLITZAR_HAS_MPI)
    if (RuntimeLive()) {
        CancelRequests(impl_->receive_requests);
        CancelRequests(impl_->send_requests);
    }

    Reset();
#else
    Reset();
#endif
}

blitzar_status MpiNative::ReserveGhost(
    MpiNativeGhost& ghost, std::size_t receive_capacity, std::size_t send_capacity) const noexcept
{
    if (ghost.impl_ == nullptr) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

#if defined(BLITZAR_HAS_MPI)

    try {
        ghost.impl_->receive_requests.reserve(receive_capacity);
        ghost.impl_->send_requests.reserve(send_capacity);
        ghost.impl_->receive_statuses.reserve(receive_capacity);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
#else

    (void)receive_capacity;
    (void)send_capacity;
#endif

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiNative::ResizeGhost(
    MpiNativeGhost& ghost, std::size_t receive_count, std::size_t send_count) const noexcept
{
    if (ghost.impl_ == nullptr) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

#if defined(BLITZAR_HAS_MPI)
    if (receive_count > ghost.impl_->receive_requests.capacity() ||
        send_count > ghost.impl_->send_requests.capacity() ||
        receive_count > ghost.impl_->receive_statuses.capacity()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        ghost.impl_->receive_requests.resize(receive_count);
        ghost.impl_->send_requests.resize(send_count);
        ghost.impl_->receive_statuses.resize(receive_count);

        ghost.impl_->receive_posted = 0;
        ghost.impl_->send_posted = 0;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
#else

    (void)receive_count;
    (void)send_count;
#endif

    return BLITZAR_STATUS_OK;
}

blitzar_status MpiNative::PostGhostReceive(
    MpiNativeGhost& ghost, const NativeGhostReceiveRequest& request) const noexcept
{
#if defined(BLITZAR_HAS_MPI)
    if (ghost.impl_ == nullptr || request.bytes < 0 || request.offset_bytes > request.wire.size() ||
        static_cast<std::size_t>(request.bytes) > request.wire.size() - request.offset_bytes ||
        ghost.impl_->receive_posted >= ghost.impl_->receive_requests.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    MPI_Request& native_request = ghost.impl_->receive_requests[ghost.impl_->receive_posted];
    native_request = MPI_REQUEST_NULL;

    if (MPI_Irecv(request.bytes == 0 ? nullptr : request.wire.data() + request.offset_bytes,
            request.bytes, MPI_BYTE, request.peer, request.tag, impl_->communicator,
            &native_request) != MPI_SUCCESS) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    ++ghost.impl_->receive_posted;

    return BLITZAR_STATUS_OK;
#else

    (void)ghost;
    (void)request;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiNative::PostGhostSend(
    MpiNativeGhost& ghost, const NativeGhostSendRequest& request) const noexcept
{
#if defined(BLITZAR_HAS_MPI)
    if (ghost.impl_ == nullptr || request.bytes < 0 || request.offset_bytes > request.wire.size() ||
        static_cast<std::size_t>(request.bytes) > request.wire.size() - request.offset_bytes ||
        ghost.impl_->send_posted >= ghost.impl_->send_requests.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    MPI_Request& native_request = ghost.impl_->send_requests[ghost.impl_->send_posted];
    native_request = MPI_REQUEST_NULL;

    if (MPI_Isend(request.bytes == 0 ? nullptr : request.wire.data() + request.offset_bytes,
            request.bytes, MPI_BYTE, request.peer, request.tag, impl_->communicator,
            &native_request) != MPI_SUCCESS) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    ++ghost.impl_->send_posted;

    return BLITZAR_STATUS_OK;
#else

    (void)ghost;
    (void)request;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiNative::WaitGhost(
    MpiNativeGhost& ghost, std::span<std::size_t> receive_bytes) const noexcept
{
#if defined(BLITZAR_HAS_MPI)
    if (ghost.impl_ == nullptr || ghost.impl_->receive_posted != receive_bytes.size() ||
        ghost.impl_->send_posted != ghost.impl_->send_requests.size() ||
        ghost.impl_->receive_statuses.size() != receive_bytes.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (ghost.impl_->receive_requests.size() > static_cast<std::size_t>(INT_MAX) ||
        ghost.impl_->send_requests.size() > static_cast<std::size_t>(INT_MAX)) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    if (!ghost.impl_->receive_requests.empty() &&
        MPI_Waitall(static_cast<int>(ghost.impl_->receive_requests.size()),
            ghost.impl_->receive_requests.data(),
            ghost.impl_->receive_statuses.data()) != MPI_SUCCESS) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    for (std::size_t index = 0; index < receive_bytes.size(); ++index) {
        int bytes = 0;

        if (MPI_Get_count(&ghost.impl_->receive_statuses[index], MPI_BYTE, &bytes) != MPI_SUCCESS ||
            bytes < 0) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        receive_bytes[index] = static_cast<std::size_t>(bytes);
    }

    if (!ghost.impl_->send_requests.empty() &&
        MPI_Waitall(static_cast<int>(ghost.impl_->send_requests.size()),
            ghost.impl_->send_requests.data(), MPI_STATUSES_IGNORE) != MPI_SUCCESS) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return BLITZAR_STATUS_OK;
#else

    (void)ghost;
    (void)receive_bytes;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

} // namespace blitzar_parallel
