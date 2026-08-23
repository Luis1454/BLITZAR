#ifndef BLITZAR_PARALLEL_MPI_SESSION_NATIVE_HPP
#define BLITZAR_PARALLEL_MPI_SESSION_NATIVE_HPP

#include "parallel/MpiSession.hpp"

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_parallel {

struct MpiSession::Impl final {
#if defined(BLITZAR_HAS_MPI)
    MPI_Comm communicator{MPI_COMM_WORLD};
    bool registered{false};
#endif
};

} // namespace blitzar_parallel

#endif
