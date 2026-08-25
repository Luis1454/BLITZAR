#include "parallel/MpiSessionNative.hpp"

#include <cstdlib>
#include <mutex>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

#if defined(BLITZAR_HAS_MPI)

namespace {

std::mutex SessionMutex;
std::size_t SessionReferences = 0;
bool InitializedByBlitzar = false;
bool FinalizerRegistered = false;

void FinalizeAtExit() noexcept
{
    std::lock_guard lock(SessionMutex);

    if (!InitializedByBlitzar) {
        return;
    }

    int initialized = 0;
    int finalized = 0;

    if (MPI_Initialized(&initialized) == MPI_SUCCESS && initialized != 0 &&
        MPI_Finalized(&finalized) == MPI_SUCCESS && finalized == 0) {
        (void)MPI_Finalize();
    }

    SessionReferences = 0;
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

void MpiSession::ReleaseMpi() noexcept
{
    std::lock_guard lock(SessionMutex);

    if (SessionReferences != 0) {
        --SessionReferences;
    }
}

#else

void MpiSession::ReleaseMpi() noexcept {}

#endif

blitzar_status MpiSession::InitializeMpi() noexcept
{
#if defined(BLITZAR_HAS_MPI)
    std::lock_guard lock(SessionMutex);
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

    ++SessionReferences;

    impl_->registered = true;
    status_ = provided < MPI_THREAD_MULTIPLE ? BLITZAR_STATUS_UNSUPPORTED : BLITZAR_STATUS_OK;

    const blitzar_status communicator_status = ReadCommunicator();

    return communicator_status == BLITZAR_STATUS_OK ? status_ : communicator_status;
#else

    return BLITZAR_STATUS_OK;
#endif
}

blitzar_status MpiSession::ReadCommunicator() noexcept
{
#if defined(BLITZAR_HAS_MPI)
    if (MPI_Comm_rank(impl_->communicator, &rank_) != MPI_SUCCESS ||
        MPI_Comm_size(impl_->communicator, &size_) != MPI_SUCCESS || size_ <= 0) {
        rank_ = 0;
        size_ = 1;

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
#endif

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_parallel
