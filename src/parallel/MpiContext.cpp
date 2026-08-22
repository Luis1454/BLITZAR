#include "parallel/MpiContext.hpp"

#include <algorithm>
#include <climits>
#include <limits>
#include <mutex>
#include <new>
#include <utility>
#include <vector>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

namespace {

constexpr int GhostCountTag = 7101;
constexpr int GhostDataTag = 7102;

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

void DrainRequests(std::vector<MPI_Request>& requests) noexcept
{
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
MpiContext::GhostExchange::~GhostExchange() noexcept = default;

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

blitzar_status MpiContext::ReduceBounds(
    std::span<blitzar_core::Scalar> minimum,
    std::span<blitzar_core::Scalar> maximum) const noexcept
{
    if (minimum.size() != 3 || maximum.size() != 3) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!IsUsable()) {
        return status_;
    }
    if (!IsDistributed()) {
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
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
            return BLITZAR_STATUS_ALLOCATION_FAILURE;
        }
    }
    GhostExchange::Impl& state = *exchange.impl_;
    if (state.active || local.size() > static_cast<std::size_t>(INT_MAX)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    try {
        state.local.assign(local.begin(), local.end());
        state.receive_counts.assign(static_cast<std::size_t>(size_), 0);
        state.offsets.assign(static_cast<std::size_t>(size_), 0);
        state.requests.clear();
        state.requests.reserve(static_cast<std::size_t>(size_ - 1) * 2);
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
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
            DrainRequests(state.requests);
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
            DrainRequests(state.requests);
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
        Reset();
        return status;
    }

    std::size_t total = 0;
    for (int peer = 0; peer < size_; ++peer) {
        const int count = state.receive_counts[static_cast<std::size_t>(peer)];
        if (count < 0 || total > std::numeric_limits<std::size_t>::max() -
                                  static_cast<std::size_t>(count)) {
            Reset();
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        state.offsets[static_cast<std::size_t>(peer)] = total;
        total += static_cast<std::size_t>(count);
    }
    try {
        ghosts.Resize(total);
        state.requests.clear();
        state.requests.reserve(static_cast<std::size_t>(size_ - 1) * 2);
    } catch (const std::bad_alloc&) {
        Reset();
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    int send_bytes = 0;
    if (!ToByteCount(state.local.size(), send_bytes)) {
        Reset();
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    for (int peer = 0; peer < size_; ++peer) {
        if (peer == rank_) {
            continue;
        }
        int receive_bytes = 0;
        if (!ToByteCount(
                static_cast<std::size_t>(
                    state.receive_counts[static_cast<std::size_t>(peer)]),
                receive_bytes)) {
            Reset();
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        state.requests.push_back(MPI_REQUEST_NULL);
        if (MPI_Irecv(
                ghosts.Data() + state.offsets[static_cast<std::size_t>(peer)],
                receive_bytes,
                MPI_BYTE,
                peer,
                GhostDataTag,
                impl_->communicator,
                &state.requests.back()) != MPI_SUCCESS) {
            DrainRequests(state.requests);
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
            DrainRequests(state.requests);
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

blitzar_status MpiContext::AllToAllCounts(
    std::span<const int> send_counts,
    std::span<int> receive_counts) const noexcept
{
    if (send_counts.size() != static_cast<std::size_t>(size_) ||
        receive_counts.size() != static_cast<std::size_t>(size_)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!IsUsable()) {
        return status_;
    }
    for (int count : send_counts) {
        if (count < 0) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }
    if (!IsDistributed()) {
        receive_counts[0] = send_counts[0];
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
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
    if (send_counts.size() != static_cast<std::size_t>(size_) ||
        send_displacements.size() != static_cast<std::size_t>(size_) ||
        receive_counts.size() != static_cast<std::size_t>(size_) ||
        receive_displacements.size() != static_cast<std::size_t>(size_) ||
        !ValidateLayout(send_counts, send_displacements, send_packets.size()) ||
        !ValidateLayout(
            receive_counts, receive_displacements, receive_packets.size())) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!IsUsable()) {
        return status_;
    }
    if (!IsDistributed()) {
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
    try {
        std::vector<int> send_bytes(static_cast<std::size_t>(size_));
        std::vector<int> receive_bytes(static_cast<std::size_t>(size_));
        std::vector<int> send_offsets(static_cast<std::size_t>(size_));
        std::vector<int> receive_offsets(static_cast<std::size_t>(size_));
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
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
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
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

blitzar_status MpiContext::AllGatherCounts(
    int local_count, std::span<int> counts) const noexcept
{
    if (local_count < 0 || counts.size() != static_cast<std::size_t>(size_)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!IsUsable()) {
        return status_;
    }
    if (!IsDistributed()) {
        counts[0] = local_count;
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
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
    if (counts.size() != static_cast<std::size_t>(size_) ||
        displacements.size() != static_cast<std::size_t>(size_) ||
        !ValidateLayout(counts, displacements, gathered_packets.size()) ||
        local_packets.size() !=
            static_cast<std::size_t>(counts[static_cast<std::size_t>(rank_)])) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!IsUsable()) {
        return status_;
    }
    if (!IsDistributed()) {
        std::copy_n(
            local_packets.begin(),
            local_packets.size(),
            gathered_packets.begin() + displacements[0]);
        return BLITZAR_STATUS_OK;
    }
#if defined(BLITZAR_HAS_MPI)
    try {
        std::vector<int> bytes(static_cast<std::size_t>(size_));
        std::vector<int> offsets(static_cast<std::size_t>(size_));
        for (int peer = 0; peer < size_; ++peer) {
            const std::size_t index = static_cast<std::size_t>(peer);
            if (!ToByteCount(
                    static_cast<std::size_t>(counts[index]), bytes[index]) ||
                !ToByteCount(
                    static_cast<std::size_t>(displacements[index]),
                    offsets[index])) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
        }
        int local_bytes = 0;
        if (!ToByteCount(local_packets.size(), local_bytes)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
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
    } catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
#else
    return BLITZAR_STATUS_INTERNAL_ERROR;
#endif
}

}  // namespace blitzar_parallel
