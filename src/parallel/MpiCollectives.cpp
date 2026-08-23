#include "parallel/MpiCollectives.hpp"

#include "parallel/MpiSessionNative.hpp"

#include <cstdio>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

namespace {

[[nodiscard]] blitzar_status NormalizeStatus(blitzar_status status) noexcept
{
    switch (status) {
    case BLITZAR_STATUS_OK:
    case BLITZAR_STATUS_INVALID_ARGUMENT:
    case BLITZAR_STATUS_ALLOCATION_FAILURE:
    case BLITZAR_STATUS_INTERNAL_ERROR:
    case BLITZAR_STATUS_SINGULARITY:
    case BLITZAR_STATUS_UNSUPPORTED:

        return status;

    default:

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
}

#if defined(BLITZAR_HAS_MPI)

[[nodiscard]] int StatusSeverity(blitzar_status status) noexcept
{
    switch (NormalizeStatus(status)) {
    case BLITZAR_STATUS_OK:

        return 0;

    case BLITZAR_STATUS_UNSUPPORTED:

        return 1;

    case BLITZAR_STATUS_INVALID_ARGUMENT:

        return 2;

    case BLITZAR_STATUS_SINGULARITY:

        return 3;

    case BLITZAR_STATUS_ALLOCATION_FAILURE:

        return 4;

    case BLITZAR_STATUS_INTERNAL_ERROR:
    default:

        return 5;
    }
}

[[nodiscard]] blitzar_status StatusFromSeverity(int severity) noexcept
{
    switch (severity) {
    case 0:

        return BLITZAR_STATUS_OK;

    case 1:

        return BLITZAR_STATUS_UNSUPPORTED;

    case 2:

        return BLITZAR_STATUS_INVALID_ARGUMENT;

    case 3:

        return BLITZAR_STATUS_SINGULARITY;

    case 4:

        return BLITZAR_STATUS_ALLOCATION_FAILURE;

    case 5:
    default:

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
}

void LogSynchronizedStatus(int rank, const char* operation, const char* phase,
    blitzar_status local_status, blitzar_status global_status) noexcept
{
    if (global_status == BLITZAR_STATUS_OK) {
        return;
    }

    std::fprintf(stderr,
        "BLITZAR MPI rank=%d operation=%s phase=%s local_status=%d "
        "global_status=%d\n",
        rank, operation == nullptr ? "unknown" : operation, phase == nullptr ? "unknown" : phase,
        static_cast<int>(NormalizeStatus(local_status)), static_cast<int>(global_status));
}

#endif

} // namespace

MpiCollectives::MpiCollectives(const MpiSession& session) noexcept : session_(session) {}

blitzar_status MpiCollectives::SynchronizeStatus(blitzar_status local_status, const char* operation,
    const char* phase, blitzar_status& global_status) const noexcept
{
    const blitzar_status normalized_status = NormalizeStatus(local_status);

    if (!session_.IsUsable()) {
        global_status = session_.Status();

        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        global_status = normalized_status;

        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)

    const int local_severity = StatusSeverity(normalized_status);
    int global_severity = 0;

    if (MPI_Allreduce(&local_severity, &global_severity, 1, MPI_INT, MPI_MAX,
            session_.Native().communicator) != MPI_SUCCESS) {
        global_status = BLITZAR_STATUS_INTERNAL_ERROR;
        LogSynchronizedStatus(session_.Rank(), operation, phase, normalized_status, global_status);

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    global_status = StatusFromSeverity(global_severity);
    LogSynchronizedStatus(session_.Rank(), operation, phase, normalized_status, global_status);

    return BLITZAR_STATUS_OK;
#else

    (void)operation;
    (void)phase;
    global_status = BLITZAR_STATUS_INTERNAL_ERROR;

    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiCollectives::ReduceBounds(
    std::span<blitzar_core::Scalar> minimum, std::span<blitzar_core::Scalar> maximum) const noexcept
{
    const bool layout_valid = minimum.size() == 3 && maximum.size() == 3;

    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        return layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT;
    }
#if defined(BLITZAR_HAS_MPI)

    blitzar_status global_layout_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        SynchronizeStatus(layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
            "MpiCollectives", "reduce-bounds-layout", global_layout_status);

    if (synchronization_status != BLITZAR_STATUS_OK || global_layout_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status
                                                           : global_layout_status;
    }
    if (MPI_Allreduce(MPI_IN_PLACE, minimum.data(), 3, MPI_DOUBLE, MPI_MIN,
            session_.Native().communicator) != MPI_SUCCESS ||
        MPI_Allreduce(MPI_IN_PLACE, maximum.data(), 3, MPI_DOUBLE, MPI_MAX,
            session_.Native().communicator) != MPI_SUCCESS) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return BLITZAR_STATUS_OK;
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiCollectives::ReduceMax(int local_value, int& global_value) const noexcept
{
    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        global_value = local_value;

        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)

    return MPI_Allreduce(&local_value, &global_value, 1, MPI_INT, MPI_MAX,
               session_.Native().communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

} // namespace blitzar_parallel
