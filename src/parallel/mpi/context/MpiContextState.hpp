#ifndef BLITZAR_PARALLEL_MPI_CONTEXT_MPI_CONTEXT_STATE_HPP
#define BLITZAR_PARALLEL_MPI_CONTEXT_MPI_CONTEXT_STATE_HPP

#include "parallel/mpi/collectives/MpiCollectives.hpp"
#include "parallel/mpi/context/MpiContext.hpp"
#include "parallel/mpi/exchange/ghost/MpiGhostTransport.hpp"
#include "parallel/mpi/exchange/packets/MpiPacketTransport.hpp"
#include "parallel/mpi/native/MpiSession.hpp"

namespace blitzar_parallel {

struct MpiContext::Impl final {
    MpiSession session;
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
