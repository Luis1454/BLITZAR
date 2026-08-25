#include "parallel/MpiNative.hpp"
#include "parallel/MpiNativeState.hpp"

#if defined(BLITZAR_HAS_MPI)

namespace blitzar_parallel {

blitzar_status MpiNative::AllToAllCounts(
    std::span<const int> send_counts, std::span<int> receive_counts) const noexcept
{
    if (send_counts.size() != static_cast<std::size_t>(size_) ||
        receive_counts.size() != static_cast<std::size_t>(size_)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return MPI_Alltoall(send_counts.data(), 1, MPI_INT, receive_counts.data(), 1, MPI_INT,
               impl_->communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MpiNative::AllGatherCounts(int local_count, std::span<int> counts) const noexcept
{
    if (counts.size() != static_cast<std::size_t>(size_)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return MPI_Allgather(&local_count, 1, MPI_INT, counts.data(), 1, MPI_INT,
               impl_->communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MpiNative::AllToAllBytes(const NativeByteAllToAllRequest& request) const noexcept
{
    if (request.send_bytes.size() != static_cast<std::size_t>(size_) ||
        request.send_offsets.size() != static_cast<std::size_t>(size_) ||
        request.receive_bytes.size() != static_cast<std::size_t>(size_) ||
        request.receive_offsets.size() != static_cast<std::size_t>(size_)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return MPI_Alltoallv(request.send_wire.data(), request.send_bytes.data(),
               request.send_offsets.data(), MPI_BYTE, request.receive_wire.data(),
               request.receive_bytes.data(), request.receive_offsets.data(), MPI_BYTE,
               impl_->communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MpiNative::AllGatherBytes(const NativeByteAllGatherRequest& request) const noexcept
{
    if (request.receive_bytes.size() != static_cast<std::size_t>(size_) ||
        request.receive_offsets.size() != static_cast<std::size_t>(size_)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return MPI_Allgatherv(request.send_wire.data(), request.send_bytes, MPI_BYTE,
               request.receive_wire.data(), request.receive_bytes.data(),
               request.receive_offsets.data(), MPI_BYTE, impl_->communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}

} // namespace blitzar_parallel

#else

namespace blitzar_parallel {

blitzar_status MpiNative::AllToAllCounts(
    std::span<const int> send_counts, std::span<int> receive_counts) const noexcept
{
    (void)send_counts;
    (void)receive_counts;

    return BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MpiNative::AllGatherCounts(int local_count, std::span<int> counts) const noexcept
{
    (void)local_count;
    (void)counts;

    return BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MpiNative::AllToAllBytes(const NativeByteAllToAllRequest& request) const noexcept
{
    (void)request;

    return BLITZAR_STATUS_INTERNAL_ERROR;
}

blitzar_status MpiNative::AllGatherBytes(const NativeByteAllGatherRequest& request) const noexcept
{
    (void)request;

    return BLITZAR_STATUS_INTERNAL_ERROR;
}

} // namespace blitzar_parallel

#endif
