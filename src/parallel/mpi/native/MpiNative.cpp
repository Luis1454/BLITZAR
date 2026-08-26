#include "parallel/mpi/native/MpiNative.hpp"

#include "parallel/mpi/native/MpiNativeState.hpp"

#include <cstdlib>
#include <mutex>
#include <new>

#if defined(BLITZAR_HAS_MPI)

namespace blitzar_parallel {

namespace {

std::mutex RuntimeMutex;
std::size_t RuntimeReferences = 0;
bool InitializedByBlitzar = false;
bool FinalizerRegistered = false;

void FinalizeAtExit() noexcept
{
    std::lock_guard lock(RuntimeMutex);

    if (!InitializedByBlitzar) {
        return;
    }

    int initialized = 0;
    int finalized = 0;

    if (MPI_Initialized(&initialized) == MPI_SUCCESS && initialized != 0 &&
        MPI_Finalized(&finalized) == MPI_SUCCESS && finalized == 0) {
        (void)MPI_Finalize();
    }

    RuntimeReferences = 0;
    InitializedByBlitzar = false;
}

[[nodiscard]] blitzar_status ValidateRuntime(int initialized) noexcept
{
    int finalized = 0;

    return initialized != 0 && (MPI_Finalized(&finalized) != MPI_SUCCESS || finalized != 0)
               ? BLITZAR_STATUS_INTERNAL_ERROR
               : BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status StartRuntime(int& provided) noexcept
{
    if (MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided) != MPI_SUCCESS) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    InitializedByBlitzar = true;

    if (!FinalizerRegistered && std::atexit(FinalizeAtExit) != 0) {
        (void)MPI_Finalize();

        InitializedByBlitzar = false;

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    FinalizerRegistered = true;

    return BLITZAR_STATUS_OK;
}

[[nodiscard]] blitzar_status AcquireRuntime(int initialized, int& provided) noexcept
{
    return initialized == 0                             ? StartRuntime(provided)
           : MPI_Query_thread(&provided) == MPI_SUCCESS ? BLITZAR_STATUS_OK
                                                        : BLITZAR_STATUS_INTERNAL_ERROR;
}

} // namespace

#else

namespace blitzar_parallel {

#endif

MpiNative::MpiNative() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        status_ = BLITZAR_STATUS_ALLOCATION_FAILURE;

        return;
    }

    status_ = Initialize();
}

MpiNative::~MpiNative() noexcept
{
    Release();
}

bool MpiNative::IsUsable() const noexcept
{
    return impl_ != nullptr && status_ == BLITZAR_STATUS_OK;
}

bool MpiNative::IsDistributed() const noexcept
{
    return IsUsable() && size_ > 1;
}

int MpiNative::Rank() const noexcept
{
    return rank_;
}

int MpiNative::Size() const noexcept
{
    return size_;
}

blitzar_status MpiNative::Status() const noexcept
{
    return status_;
}

blitzar_status MpiNative::Initialize() noexcept
{
#if defined(BLITZAR_HAS_MPI)
    std::lock_guard lock(RuntimeMutex);
    int initialized = 0;

    if (MPI_Initialized(&initialized) != MPI_SUCCESS ||
        ValidateRuntime(initialized) != BLITZAR_STATUS_OK) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    int provided = MPI_THREAD_SINGLE;
    const blitzar_status runtime_status = AcquireRuntime(initialized, provided);

    if (runtime_status != BLITZAR_STATUS_OK) {
        return runtime_status;
    }

    ++RuntimeReferences;

    impl_->registered = true;

    if (MPI_Comm_rank(impl_->communicator, &rank_) != MPI_SUCCESS ||
        MPI_Comm_size(impl_->communicator, &size_) != MPI_SUCCESS || size_ <= 0) {
        rank_ = 0;
        size_ = 1;

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    return provided < MPI_THREAD_MULTIPLE ? BLITZAR_STATUS_UNSUPPORTED : BLITZAR_STATUS_OK;
#else
    return BLITZAR_STATUS_OK;
#endif
}

void MpiNative::Release() noexcept
{
#if defined(BLITZAR_HAS_MPI)
    if (impl_ == nullptr || !impl_->registered) {
        return;
    }

    std::lock_guard lock(RuntimeMutex);

    if (RuntimeReferences != 0) {
        --RuntimeReferences;
    }

    impl_->registered = false;
#endif
}

} // namespace blitzar_parallel
