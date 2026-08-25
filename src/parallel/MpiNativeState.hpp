#ifndef BLITZAR_PARALLEL_MPI_NATIVE_STATE_HPP
#define BLITZAR_PARALLEL_MPI_NATIVE_STATE_HPP

#include "parallel/MpiNative.hpp"

#include <vector>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

struct MpiNative::Impl final {
#if defined(BLITZAR_HAS_MPI)
    MPI_Comm communicator{MPI_COMM_WORLD};
    bool registered{false};
#endif
};

struct MpiNativeGhost::Impl final {
#if defined(BLITZAR_HAS_MPI)
    std::vector<MPI_Request> receive_requests;
    std::vector<MPI_Request> send_requests;
    std::vector<MPI_Status> receive_statuses;
    std::size_t receive_posted{0};
    std::size_t send_posted{0};
#endif
};

} // namespace blitzar_parallel

#endif
