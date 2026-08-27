#ifndef BLITZAR_MPI_RUNTIME_MPI_STATE_HPP
#define BLITZAR_MPI_RUNTIME_MPI_STATE_HPP

#include "mpi/collectives/MpiCollectives.hpp"
#include "mpi/ghost/MpiGhostTransport.hpp"
#include "mpi/native/MpiNativeSession.hpp"
#include "mpi/packets/MpiPacketTransport.hpp"
#include "mpi/runtime/MpiContext.hpp"

namespace blitzar_parallel {

struct MpiContext::Impl final {
    MpiNativeSession session;
    MpiCollectives collectives;
    MpiPacketTransport packets;
    MpiGhostTransport ghosts;

    Impl() noexcept
        : session(), collectives(session), packets(session, collectives),
          ghosts(session, collectives, packets)
    {
    }
};

} // namespace blitzar_parallel

#endif
