#include "parallel/MpiContext.hpp"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <limits>
#include <mutex>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

namespace {

constexpr int GhostCountTag = 7101;
constexpr int GhostDataTag = 7102;

[[nodiscard]] blitzar_status NormalizeStatus(
    blitzar_status status) noexcept
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

void LogSynchronizedStatus(
    int rank,
    const char* operation,
    const char* phase,
    blitzar_status local_status,
    blitzar_status global_status) noexcept
{
    if (global_status == BLITZAR_STATUS_OK) {
        return;
    }
    std::fprintf(
        stderr,
        "BLITZAR MPI rank=%d operation=%s phase=%s local_status=%d "
        "global_status=%d\n",
        rank,
        operation == nullptr ? "unknown" : operation,
        phase == nullptr ? "unknown" : phase,
        static_cast<int>(NormalizeStatus(local_status)),
        static_cast<int>(global_status));
}

#endif

[[nodiscard]] bool ValidateLayout(
    std::span<const int> counts,
    std::span<const int> displacements,
    std::size_t packet_count) noexcept
{
    if (counts.size() != displacements.size()) {
        return false;
    }
    for (std::size_t index = 0; index < counts.size(); ++index) {
        if (counts[index] < 0 || displacements[index] < 0 ||
            static_cast<std::size_t>(displacements[index]) > packet_count ||
            static_cast<std::size_t>(counts[index]) >
                packet_count - static_cast<std::size_t>(displacements[index])) {
            return false;
        }
    }
    return true;
}

#if defined(BLITZAR_HAS_MPI)

[[nodiscard]] bool ToByteCount(std::size_t packets, int& bytes) noexcept
{
    if (packets > static_cast<std::size_t>(INT_MAX) /
                          sizeof(ParticlePacket)) {
        return false;
    }
    bytes = static_cast<int>(packets * sizeof(ParticlePacket));
    return true;
}

std::mutex ContextMutex;
std::size_t ContextReferences = 0;
bool InitializedByBlitzar = false;

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

struct MpiContext::Impl final {
#if defined(BLITZAR_HAS_MPI)
    MPI_Comm communicator{MPI_COMM_WORLD};
    bool registered{false};
#endif
};

struct MpiContext::GhostExchange::Impl final {
    bool active{false};
#if defined(BLITZAR_HAS_MPI)
    std::vector<ParticlePacket> local;
    int local_count{0};
    std::vector<int> receive_counts;
    std::vector<std::size_t> offsets;
    std::vector<MPI_Request> requests;
#endif
};

MpiContext::GhostExchange::GhostExchange() noexcept = default;
MpiContext::GhostExchange::~GhostExchange() noexcept
{
#if defined(BLITZAR_HAS_MPI)
    if (impl_ == nullptr || !impl_->active) {
        return;
    }
    int initialized = 0;
    int finalized = 0;
    if (MPI_Initialized(&initialized) != MPI_SUCCESS || initialized == 0 ||
        MPI_Finalized(&finalized) != MPI_SUCCESS || finalized != 0) {
        impl_->requests.clear();
    } else {
        CancelRequests(impl_->requests);
    }
    impl_->active = false;
    impl_->local.clear();
    impl_->receive_counts.clear();
    impl_->offsets.clear();
#endif
}

MpiContext::GhostExchange::GhostExchange(GhostExchange&& other) noexcept =
    default;

MpiContext::GhostExchange& MpiContext::GhostExchange::operator=(
    GhostExchange&& other) noexcept = default;

MpiContext::MpiContext() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    } catch (const std::bad_alloc&) {
        status_ = BLITZAR_STATUS_ALLOCATION_FAILURE;
        return;
    }

#if defined(BLITZAR_HAS_MPI)
    std::lock_guard lock(ContextMutex);
    int initialized = 0;
    if (MPI_Initialized(&initialized) != MPI_SUCCESS) {
        status_ = BLITZAR_STATUS_INTERNAL_ERROR;
        return;
    }

    int provided = MPI_THREAD_SINGLE;
    if (initialized == 0) {
        if (MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided) !=
            MPI_SUCCESS) {
            status_ = BLITZAR_STATUS_INTERNAL_ERROR;
            return;
        }
        InitializedByBlitzar = true;
    } else if (MPI_Query_thread(&provided) != MPI_SUCCESS) {
        status_ = BLITZAR_STATUS_INTERNAL_ERROR;
        return;
    }
    ++ContextReferences;
    impl_->registered = true;

    if (provided < MPI_THREAD_MULTIPLE) {
        status_ = BLITZAR_STATUS_UNSUPPORTED;
    }
    if (MPI_Comm_rank(impl_->communicator, &rank_) != MPI_SUCCESS ||
        MPI_Comm_size(impl_->communicator, &size_) != MPI_SUCCESS || size_ <= 0) {
        status_ = BLITZAR_STATUS_INTERNAL_ERROR;
        rank_ = 0;
        size_ = 1;
    }
#endif
}

MpiContext::~MpiContext() noexcept
{
#if defined(BLITZAR_HAS_MPI)
    if (impl_ == nullptr) {
        return;
    }
    std::lock_guard lock(ContextMutex);
    if (!impl_->registered || ContextReferences == 0) {
        return;
    }
    --ContextReferences;
    if (ContextReferences != 0 || !InitializedByBlitzar) {
        return;
    }

    int finalized = 0;
    if (MPI_Finalized(&finalized) == MPI_SUCCESS && finalized == 0) {
        MPI_Finalize();
    }
    InitializedByBlitzar = false;
#endif
}

bool MpiContext::IsUsable() const noexcept
{
    return status_ == BLITZAR_STATUS_OK;
}

bool MpiContext::IsDistributed() const noexcept
{
    return IsUsable() && size_ > 1;
}

int MpiContext::Rank() const noexcept
{
    return rank_;
}

int MpiContext::Size() const noexcept
{
    return size_;
}

blitzar_status MpiContext::Status() const noexcept
{
    return status_;
}

blitzar_status MpiContext::SynchronizeStatus(
    blitzar_status local_status,
    const char* operation,
    const char* phase,
    blitzar_status& global_status) const noexcept
{
    const blitzar_status normalized_status = NormalizeStatus(local_status);
    if (!IsUsable()) {
        global_status = status_;
        return status_;
    }
    if (!IsDistributed()) {
        global_status = normalized_status;
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    const int local_severity = StatusSeverity(normalized_status);
    int global_severity = 0;
    if (MPI_Allreduce(
            &local_severity,
            &global_severity,
            1,
            MPI_INT,
            MPI_MAX,
            impl_->communicator) != MPI_SUCCESS) {
        global_status = BLITZAR_STATUS_INTERNAL_ERROR;
        LogSynchronizedStatus(
            rank_,
            operation,
            phase,
            normalized_status,
            global_status);
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    global_status = StatusFromSeverity(global_severity);
    LogSynchronizedStatus(
        rank_,
        operation,
        phase,
        normalized_status,
        global_status);
    return BLITZAR_STATUS_OK;
#else
    (void)operation;
    (void)phase;
    global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiContext::ReduceBounds(
    std::span<blitzar_core::Scalar> minimum,
    std::span<blitzar_core::Scalar> maximum) const noexcept
{
    const bool layout_valid = minimum.size() == 3 && maximum.size() == 3;
    if (!IsUsable()) {
        return status_;
    }
    if (!IsDistributed()) {
        return layout_valid ? BLITZAR_STATUS_OK
                            : BLITZAR_STATUS_INVALID_ARGUMENT;
    }
#if defined(BLITZAR_HAS_MPI)
    blitzar_status global_layout_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = SynchronizeStatus(
        layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "MpiContext",
        "reduce-bounds-layout",
        global_layout_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_layout_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_layout_status;
    }
    if (MPI_Allreduce(
            MPI_IN_PLACE,
            minimum.data(),
            3,
            MPI_DOUBLE,
            MPI_MIN,
            impl_->communicator) != MPI_SUCCESS ||
        MPI_Allreduce(
            MPI_IN_PLACE,
            maximum.data(),
            3,
            MPI_DOUBLE,
            MPI_MAX,
            impl_->communicator) != MPI_SUCCESS) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
    return BLITZAR_STATUS_OK;
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiContext::ReduceMax(
    int local_value, int& global_value) const noexcept
{
    if (!IsUsable()) {
        return status_;
    }
    if (!IsDistributed()) {
        global_value = local_value;
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    return MPI_Allreduce(
               &local_value,
               &global_value,
               1,
               MPI_INT,
               MPI_MAX,
               impl_->communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiContext::BeginGhostExchange(
    std::span<const ParticlePacket> local,
    GhostExchange& exchange) const noexcept
{
    if (!IsUsable()) {
        return status_;
    }
    if (!IsDistributed()) {
        exchange.impl_.reset();
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    if (exchange.impl_ == nullptr) {
        try {
            exchange.impl_ = std::make_unique<GhostExchange::Impl>();
        } catch (const std::bad_alloc&) {
            blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
            const blitzar_status synchronization_status = SynchronizeStatus(
                BLITZAR_STATUS_ALLOCATION_FAILURE,
                "MpiContext",
                "ghost-begin-prepare",
                global_status);
            return synchronization_status != BLITZAR_STATUS_OK
                       ? synchronization_status
                       : global_status;
        }
    }
    GhostExchange::Impl& state = *exchange.impl_;
    blitzar_status preparation_status =
        state.active || local.size() > static_cast<std::size_t>(INT_MAX)
            ? BLITZAR_STATUS_INVALID_ARGUMENT
            : BLITZAR_STATUS_OK;
    try {
        if (preparation_status == BLITZAR_STATUS_OK) {
            state.local.assign(local.begin(), local.end());
            state.receive_counts.assign(static_cast<std::size_t>(size_), 0);
            state.offsets.assign(static_cast<std::size_t>(size_), 0);
            state.requests.clear();
            state.requests.reserve(static_cast<std::size_t>(size_ - 1) * 2);
        }
    } catch (const std::length_error&) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    blitzar_status global_preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = SynchronizeStatus(
        preparation_status,
        "MpiContext",
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
    for (int peer = 0; peer < size_; ++peer) {
        if (peer == rank_) {
            continue;
        }
        state.requests.push_back(MPI_REQUEST_NULL);
        if (MPI_Irecv(
                &state.receive_counts[static_cast<std::size_t>(peer)],
                1,
                MPI_INT,
                peer,
                GhostCountTag,
                impl_->communicator,
                &state.requests.back()) != MPI_SUCCESS) {
            CancelRequests(state.requests);
            state.active = false;
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        state.requests.push_back(MPI_REQUEST_NULL);
        if (MPI_Isend(
                &state.local_count,
                1,
                MPI_INT,
                peer,
                GhostCountTag,
                impl_->communicator,
                &state.requests.back()) != MPI_SUCCESS) {
            CancelRequests(state.requests);
            state.active = false;
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

blitzar_status MpiContext::CompleteGhostExchange(
    GhostExchange& exchange, PacketBuffer& ghosts) const noexcept
{
    ghosts.Clear();
    if (!IsUsable()) {
        return status_;
    }
    if (!IsDistributed()) {
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    const bool exchange_active =
        exchange.impl_ != nullptr && exchange.impl_->active;
    blitzar_status global_active_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status active_synchronization_status = SynchronizeStatus(
        exchange_active ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "MpiContext",
        "ghost-complete-preflight",
        global_active_status);
    if (active_synchronization_status != BLITZAR_STATUS_OK ||
        global_active_status != BLITZAR_STATUS_OK) {
        AbortGhostExchange(exchange);
        return active_synchronization_status != BLITZAR_STATUS_OK
                   ? active_synchronization_status
                   : global_active_status;
    }
    if (exchange.impl_ == nullptr || !exchange.impl_->active) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    GhostExchange::Impl& state = *exchange.impl_;
    const auto Reset = [&state]() noexcept {
        state.active = false;
        state.requests.clear();
        state.local.clear();
    };
    blitzar_status status = WaitRequests(state.requests);
    if (status != BLITZAR_STATUS_OK) {
        CancelRequests(state.requests);
        Reset();
        return status;
    }

    std::size_t total = 0;
    blitzar_status count_status = BLITZAR_STATUS_OK;
    for (int peer = 0; peer < size_; ++peer) {
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
    const blitzar_status count_synchronization_status = SynchronizeStatus(
        count_status,
        "MpiContext",
        "ghost-count-prepare",
        global_count_status);
    if (count_synchronization_status != BLITZAR_STATUS_OK ||
        global_count_status != BLITZAR_STATUS_OK) {
        Reset();
        return count_synchronization_status != BLITZAR_STATUS_OK
                   ? count_synchronization_status
                   : global_count_status;
    }
    int send_bytes = 0;
    std::vector<int> receive_bytes;
    blitzar_status preparation_status = BLITZAR_STATUS_OK;
    try {
        ghosts.Resize(total);
        state.requests.clear();
        state.requests.reserve(static_cast<std::size_t>(size_ - 1) * 2);
        receive_bytes.assign(static_cast<std::size_t>(size_), 0);
    } catch (const std::length_error&) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    if (preparation_status == BLITZAR_STATUS_OK &&
        !ToByteCount(state.local.size(), send_bytes)) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    for (int peer = 0; peer < size_; ++peer) {
        if (preparation_status != BLITZAR_STATUS_OK || peer == rank_) {
            continue;
        }
        if (!ToByteCount(
                static_cast<std::size_t>(
                    state.receive_counts[static_cast<std::size_t>(peer)]),
                receive_bytes[static_cast<std::size_t>(peer)])) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }
    blitzar_status global_preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = SynchronizeStatus(
        preparation_status,
        "MpiContext",
        "ghost-data-prepare",
        global_preparation_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_preparation_status != BLITZAR_STATUS_OK) {
        Reset();
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_preparation_status;
    }

    for (int peer = 0; peer < size_; ++peer) {
        if (peer == rank_) {
            continue;
        }
        state.requests.push_back(MPI_REQUEST_NULL);
        if (MPI_Irecv(
                ghosts.Data() + state.offsets[static_cast<std::size_t>(peer)],
                receive_bytes[static_cast<std::size_t>(peer)],
                MPI_BYTE,
                peer,
                GhostDataTag,
                impl_->communicator,
                &state.requests.back()) != MPI_SUCCESS) {
            CancelRequests(state.requests);
            Reset();
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        state.requests.push_back(MPI_REQUEST_NULL);
        if (MPI_Isend(
                state.local.data(),
                send_bytes,
                MPI_BYTE,
                peer,
                GhostDataTag,
                impl_->communicator,
                &state.requests.back()) != MPI_SUCCESS) {
            CancelRequests(state.requests);
            Reset();
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
    }
    status = WaitRequests(state.requests);
    Reset();
    return status;
#else
    (void)exchange;
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

bool MpiContext::IsGhostExchangeActive(
    const GhostExchange& exchange) const noexcept
{
#if defined(BLITZAR_HAS_MPI)
    return exchange.impl_ != nullptr && exchange.impl_->active;
#else
    (void)exchange;
    return false;
#endif
}

void MpiContext::AbortGhostExchange(GhostExchange& exchange) const noexcept
{
    if (exchange.impl_ == nullptr) {
        return;
    }
#if defined(BLITZAR_HAS_MPI)
    GhostExchange::Impl& state = *exchange.impl_;
    if (state.active) {
        CancelRequests(state.requests);
    } else {
        state.requests.clear();
    }
    state.active = false;
    state.local.clear();
    state.receive_counts.clear();
    state.offsets.clear();
#else
    exchange.impl_.reset();
#endif
}

blitzar_status MpiContext::AllToAllCounts(
    std::span<const int> send_counts,
    std::span<int> receive_counts) const noexcept
{
    bool layout_valid =
        send_counts.size() == static_cast<std::size_t>(size_) &&
        receive_counts.size() == static_cast<std::size_t>(size_);
    if (layout_valid) {
        for (const int count : send_counts) {
            if (count < 0) {
                layout_valid = false;
                break;
            }
        }
    }
    if (!IsUsable()) {
        return status_;
    }
    if (!IsDistributed()) {
        if (!layout_valid) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        receive_counts[0] = send_counts[0];
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    blitzar_status global_layout_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = SynchronizeStatus(
        layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "MpiContext",
        "alltoall-count-layout",
        global_layout_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_layout_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_layout_status;
    }
    return MPI_Alltoall(
               send_counts.data(),
               1,
               MPI_INT,
               receive_counts.data(),
               1,
               MPI_INT,
               impl_->communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiContext::AllToAllPackets(
    std::span<const ParticlePacket> send_packets,
    std::span<const int> send_counts,
    std::span<const int> send_displacements,
    std::span<ParticlePacket> receive_packets,
    std::span<const int> receive_counts,
    std::span<const int> receive_displacements) const noexcept
{
    const bool layout_valid =
        send_counts.size() == static_cast<std::size_t>(size_) &&
        send_displacements.size() == static_cast<std::size_t>(size_) &&
        receive_counts.size() == static_cast<std::size_t>(size_) &&
        receive_displacements.size() == static_cast<std::size_t>(size_) &&
        ValidateLayout(send_counts, send_displacements, send_packets.size()) &&
        ValidateLayout(
            receive_counts, receive_displacements, receive_packets.size());
    if (!IsUsable()) {
        return status_;
    }
    if (!IsDistributed()) {
        if (!layout_valid) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        if (send_counts[0] != receive_counts[0]) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        std::copy_n(
            send_packets.begin() + send_displacements[0],
            send_counts[0],
            receive_packets.begin() + receive_displacements[0]);
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    blitzar_status preparation_status = layout_valid
                                            ? BLITZAR_STATUS_OK
                                            : BLITZAR_STATUS_INVALID_ARGUMENT;
    blitzar_status global_preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
    blitzar_status synchronization_status = SynchronizeStatus(
        preparation_status,
        "MpiContext",
        "alltoall-packet-layout",
        global_preparation_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_preparation_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_preparation_status;
    }
    std::vector<int> send_bytes;
    std::vector<int> receive_bytes;
    std::vector<int> send_offsets;
    std::vector<int> receive_offsets;
    try {
        send_bytes.assign(static_cast<std::size_t>(size_), 0);
        receive_bytes.assign(static_cast<std::size_t>(size_), 0);
        send_offsets.assign(static_cast<std::size_t>(size_), 0);
        receive_offsets.assign(static_cast<std::size_t>(size_), 0);
        for (int peer = 0; peer < size_; ++peer) {
            const std::size_t index = static_cast<std::size_t>(peer);
            if (!ToByteCount(
                    static_cast<std::size_t>(send_counts[index]),
                    send_bytes[index]) ||
                !ToByteCount(
                    static_cast<std::size_t>(receive_counts[index]),
                    receive_bytes[index]) ||
                !ToByteCount(
                    static_cast<std::size_t>(send_displacements[index]),
                    send_offsets[index]) ||
                !ToByteCount(
                     static_cast<std::size_t>(receive_displacements[index]),
                     receive_offsets[index])) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                break;
            }
        }
    } catch (const std::length_error&) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    synchronization_status = SynchronizeStatus(
        preparation_status,
        "MpiContext",
        "alltoall-packet-prepare",
        global_preparation_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_preparation_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_preparation_status;
    }
    return MPI_Alltoallv(
                   send_packets.data(),
                   send_bytes.data(),
                   send_offsets.data(),
                   MPI_BYTE,
                   receive_packets.data(),
                   receive_bytes.data(),
                   receive_offsets.data(),
                   MPI_BYTE,
                   impl_->communicator) == MPI_SUCCESS
                   ? BLITZAR_STATUS_OK
                   : BLITZAR_STATUS_INTERNAL_ERROR;
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiContext::AllGatherCounts(
    int local_count, std::span<int> counts) const noexcept
{
    const bool layout_valid =
        local_count >= 0 && counts.size() == static_cast<std::size_t>(size_);
    if (!IsUsable()) {
        return status_;
    }
    if (!IsDistributed()) {
        if (!layout_valid) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        counts[0] = local_count;
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    blitzar_status global_layout_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = SynchronizeStatus(
        layout_valid ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT,
        "MpiContext",
        "allgather-count-layout",
        global_layout_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_layout_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_layout_status;
    }
    return MPI_Allgather(
               &local_count,
               1,
               MPI_INT,
               counts.data(),
               1,
               MPI_INT,
               impl_->communicator) == MPI_SUCCESS
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiContext::AllGatherPackets(
    std::span<const ParticlePacket> local_packets,
    std::span<ParticlePacket> gathered_packets,
    std::span<const int> counts,
    std::span<const int> displacements) const noexcept
{
    const bool layout_valid =
        counts.size() == static_cast<std::size_t>(size_) &&
        displacements.size() == static_cast<std::size_t>(size_) &&
        ValidateLayout(counts, displacements, gathered_packets.size());
    const bool local_count_valid =
        layout_valid &&
        local_packets.size() ==
            static_cast<std::size_t>(counts[static_cast<std::size_t>(rank_)]);
    if (!IsUsable()) {
        return status_;
    }
    if (!IsDistributed()) {
        if (!local_count_valid) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        std::copy_n(
            local_packets.begin(),
            local_packets.size(),
            gathered_packets.begin() + displacements[0]);
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    blitzar_status preparation_status =
        layout_valid && local_count_valid ? BLITZAR_STATUS_OK
                                          : BLITZAR_STATUS_INVALID_ARGUMENT;
    blitzar_status global_preparation_status = BLITZAR_STATUS_INTERNAL_ERROR;
    blitzar_status synchronization_status = SynchronizeStatus(
        preparation_status,
        "MpiContext",
        "allgather-packet-layout",
        global_preparation_status);
    if (synchronization_status != BLITZAR_STATUS_OK ||
        global_preparation_status != BLITZAR_STATUS_OK) {
        return synchronization_status != BLITZAR_STATUS_OK
                   ? synchronization_status
                   : global_preparation_status;
    }
    std::vector<int> bytes;
    std::vector<int> offsets;
    try {
        bytes.assign(static_cast<std::size_t>(size_), 0);
        offsets.assign(static_cast<std::size_t>(size_), 0);
        for (int peer = 0; peer < size_; ++peer) {
            const std::size_t index = static_cast<std::size_t>(peer);
            if (!ToByteCount(
                    static_cast<std::size_t>(counts[index]), bytes[index]) ||
                !ToByteCount(
                     static_cast<std::size_t>(displacements[index]),
                     offsets[index])) {
                preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
                break;
            }
        }
        int local_bytes = 0;
        if (preparation_status == BLITZAR_STATUS_OK &&
            !ToByteCount(local_packets.size(), local_bytes)) {
            preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        synchronization_status = SynchronizeStatus(
            preparation_status,
            "MpiContext",
            "allgather-packet-prepare",
            global_preparation_status);
        if (synchronization_status != BLITZAR_STATUS_OK ||
            global_preparation_status != BLITZAR_STATUS_OK) {
            return synchronization_status != BLITZAR_STATUS_OK
                       ? synchronization_status
                       : global_preparation_status;
        }
        return MPI_Allgatherv(
                   local_packets.data(),
                   local_bytes,
                   MPI_BYTE,
                   gathered_packets.data(),
                   bytes.data(),
                   offsets.data(),
                   MPI_BYTE,
                   impl_->communicator) == MPI_SUCCESS
                   ? BLITZAR_STATUS_OK
                   : BLITZAR_STATUS_INTERNAL_ERROR;
    } catch (const std::length_error&) {
        preparation_status = BLITZAR_STATUS_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        preparation_status = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    synchronization_status = SynchronizeStatus(
        preparation_status,
        "MpiContext",
        "allgather-packet-prepare",
        global_preparation_status);
    return synchronization_status != BLITZAR_STATUS_OK
               ? synchronization_status
               : global_preparation_status;
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

}  // namespace blitzar_parallel
