#include "parallel/MpiPacketProtocol.hpp"
#include "parallel/MpiPacketTransport.hpp"
#include "parallel/MpiSessionNative.hpp"

#include <cstddef>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

namespace {

[[nodiscard]] bool ValidAllToAllCounts(std::span<const int> send_counts,
    std::span<int> receive_counts, std::size_t peer_count) noexcept
{
    if (send_counts.size() != peer_count || receive_counts.size() != peer_count) {
        return false;
    }

    for (const int count : send_counts) {
        if (count < 0) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool ValidAllGatherCount(
    int local_count, std::span<int> counts, std::size_t peer_count) noexcept
{
    return local_count >= 0 && counts.size() == peer_count;
}

} // namespace

blitzar_status MpiPacketTransport::AllToAllCounts(
    std::span<const int> send_counts, std::span<int> receive_counts) const noexcept
{
    const bool layout_valid =
        ValidAllToAllCounts(send_counts, receive_counts, static_cast<std::size_t>(session_.Size()));

    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        if (!layout_valid) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        receive_counts[0] = send_counts[0];

        return BLITZAR_STATUS_OK;
    }

    return ExecuteAllToAllCounts(layout_valid, send_counts, receive_counts);
}

blitzar_status MpiPacketTransport::ExecuteAllToAllCounts(bool layout_valid,
    std::span<const int> send_counts, std::span<int> receive_counts) const noexcept
{
#if defined(BLITZAR_HAS_MPI)
    const blitzar_status status = MpiPacketProtocol::SynchronizePreparation(collectives_,
        layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "alltoall-count-layout");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    return MPI_Alltoall(send_counts.data(), 1, MPI_INT, receive_counts.data(), 1, MPI_INT,
               session_.Native().communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
#else

    (void)layout_valid;
    (void)send_counts;
    (void)receive_counts;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiPacketTransport::AllGatherCounts(
    int local_count, std::span<int> counts) const noexcept
{
    const bool layout_valid =
        ValidAllGatherCount(local_count, counts, static_cast<std::size_t>(session_.Size()));

    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        if (!layout_valid) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        counts[0] = local_count;

        return BLITZAR_STATUS_OK;
    }

    return ExecuteAllGatherCounts(layout_valid, local_count, counts);
}

blitzar_status MpiPacketTransport::ExecuteAllGatherCounts(
    bool layout_valid, int local_count, std::span<int> counts) const noexcept
{
#if defined(BLITZAR_HAS_MPI)
    const blitzar_status status = MpiPacketProtocol::SynchronizePreparation(collectives_,
        layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "allgather-count-layout");

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    return MPI_Allgather(&local_count, 1, MPI_INT, counts.data(), 1, MPI_INT,
               session_.Native().communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
#else

    (void)layout_valid;
    (void)local_count;
    (void)counts;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

} // namespace blitzar_parallel
