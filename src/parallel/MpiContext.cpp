#include "parallel/MpiContext.hpp"

#if defined(BLITZAR_HAS_MPI)

#include <mutex>

namespace blitzar_parallel {

namespace {

std::mutex ContextMutex;
std::size_t ContextReferences = 0;
bool InitializedByBlitzar = false;

}  // namespace

MpiContext::MpiContext() noexcept
{
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
    }
    ++ContextReferences;
    registered_ = true;

    if (status_ == BLITZAR_STATUS_OK && provided < MPI_THREAD_MULTIPLE) {
        status_ = BLITZAR_STATUS_UNSUPPORTED;
    }
    if (MPI_Comm_rank(communicator_, &rank_) != MPI_SUCCESS ||
        MPI_Comm_size(communicator_, &size_) != MPI_SUCCESS || size_ <= 0) {
        status_ = BLITZAR_STATUS_INTERNAL_ERROR;
        rank_ = 0;
        size_ = 1;
    }
}

MpiContext::~MpiContext() noexcept
{
    std::lock_guard lock(ContextMutex);
    if (!registered_ || ContextReferences == 0) {
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

MpiCommunicator MpiContext::Communicator() const noexcept
{
    return communicator_;
}

blitzar_status MpiContext::Status() const noexcept
{
    return status_;
}

}  // namespace blitzar_parallel

#else

namespace blitzar_parallel {

MpiContext::MpiContext() noexcept = default;
MpiContext::~MpiContext() noexcept = default;

bool MpiContext::IsUsable() const noexcept
{
    return true;
}

bool MpiContext::IsDistributed() const noexcept
{
    return false;
}

int MpiContext::Rank() const noexcept
{
    return rank_;
}

int MpiContext::Size() const noexcept
{
    return size_;
}

MpiCommunicator MpiContext::Communicator() const noexcept
{
    return 0;
}

blitzar_status MpiContext::Status() const noexcept
{
    return status_;
}

}  // namespace blitzar_parallel

#endif
