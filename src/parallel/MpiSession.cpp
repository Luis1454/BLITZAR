#include "parallel/MpiSessionNative.hpp"

#include <mutex>
#include <new>

namespace blitzar_parallel {

namespace {

#if defined(BLITZAR_HAS_MPI)

std::mutex SessionMutex;
std::size_t SessionReferences = 0;
bool InitializedByBlitzar = false;

#endif

} // namespace

MpiSession::MpiSession() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        status_ = BLITZAR_STATUS_ALLOCATION_FAILURE;

        return;
    }

#if defined(BLITZAR_HAS_MPI)

    std::lock_guard lock(SessionMutex);
    int initialized = 0;

    if (MPI_Initialized(&initialized) != MPI_SUCCESS) {
        status_ = BLITZAR_STATUS_INTERNAL_ERROR;

        return;
    }

    int provided = MPI_THREAD_SINGLE;

    if (initialized == 0) {
        if (MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided) != MPI_SUCCESS) {
            status_ = BLITZAR_STATUS_INTERNAL_ERROR;

            return;
        }

        InitializedByBlitzar = true;
    }
    else if (MPI_Query_thread(&provided) != MPI_SUCCESS) {
        status_ = BLITZAR_STATUS_INTERNAL_ERROR;

        return;
    }

    ++SessionReferences;

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

MpiSession::~MpiSession() noexcept
{
#if defined(BLITZAR_HAS_MPI)
    if (impl_ == nullptr) {
        return;
    }

    std::lock_guard lock(SessionMutex);

    if (!impl_->registered || SessionReferences == 0) {
        return;
    }

    --SessionReferences;

    if (SessionReferences != 0 || !InitializedByBlitzar) {
        return;
    }

    int finalized = 0;

    if (MPI_Finalized(&finalized) == MPI_SUCCESS && finalized == 0) {
        MPI_Finalize();
    }

    InitializedByBlitzar = false;
#endif
}

bool MpiSession::IsUsable() const noexcept
{
    return status_ == BLITZAR_STATUS_OK;
}

bool MpiSession::IsDistributed() const noexcept
{
    return IsUsable() && size_ > 1;
}

int MpiSession::Rank() const noexcept
{
    return rank_;
}

int MpiSession::Size() const noexcept
{
    return size_;
}

blitzar_status MpiSession::Status() const noexcept
{
    return status_;
}

const MpiSession::Impl& MpiSession::Native() const noexcept
{
    return *impl_;
}

} // namespace blitzar_parallel
