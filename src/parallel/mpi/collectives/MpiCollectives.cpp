#include "parallel/mpi/collectives/MpiCollectives.hpp"

#include <algorithm>
#include <climits>
#include <cstdio>

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

struct StatusLogRequest final {
    int rank;
    std::string_view operation;
    std::string_view phase;
    blitzar_status local_status;
    blitzar_status global_status;
};

void LogSynchronizedStatus(const StatusLogRequest& request) noexcept
{
    if (request.global_status == BLITZAR_STATUS_OK) {
        return;
    }

    const std::string_view operation = request.operation.empty() ? "unknown" : request.operation;
    const std::string_view phase = request.phase.empty() ? "unknown" : request.phase;
    const int operation_size =
        static_cast<int>(std::min(operation.size(), static_cast<std::size_t>(INT_MAX)));

    const int phase_size =
        static_cast<int>(std::min(phase.size(), static_cast<std::size_t>(INT_MAX)));

    std::fprintf(stderr,
        "BLITZAR MPI rank=%d operation=%.*s phase=%.*s local_status=%d "
        "global_status=%d\n",
        request.rank, operation_size, operation.data(), phase_size, phase.data(),
        static_cast<int>(NormalizeStatus(request.local_status)),
        static_cast<int>(request.global_status));
}

} // namespace

MpiCollectives::MpiCollectives(const MpiSession& session) noexcept : session_(session) {}

blitzar_status MpiCollectives::SynchronizeStatus(blitzar_status local_status,
    std::string_view operation, std::string_view phase,
    blitzar_status& global_status) const noexcept
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

    const int local_severity = StatusSeverity(normalized_status);
    int global_severity = 0;

    if (session_.Native().ReduceMaxInt(local_severity, global_severity) != BLITZAR_STATUS_OK) {
        global_status = BLITZAR_STATUS_INTERNAL_ERROR;

        LogSynchronizedStatus(
            {session_.Rank(), operation, phase, normalized_status, global_status});

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    global_status = StatusFromSeverity(global_severity);

    LogSynchronizedStatus({session_.Rank(), operation, phase, normalized_status, global_status});

    return BLITZAR_STATUS_OK;
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

    blitzar_status global_layout_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status =
        SynchronizeStatus(layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
            "MpiCollectives", "reduce-bounds-layout", global_layout_status);

    if (synchronization_status != BLITZAR_STATUS_OK || global_layout_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status
                                                           : global_layout_status;
    }

    return session_.Native().ReduceBounds(minimum, maximum);
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

    return session_.Native().ReduceMaxInt(local_value, global_value);
}

} // namespace blitzar_parallel
