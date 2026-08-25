#ifndef BLITZAR_PARALLEL_MPI_CONTEXT_STATE_HPP
#define BLITZAR_PARALLEL_MPI_CONTEXT_STATE_HPP

#include "parallel/MpiCollectives.hpp"
#include "parallel/MpiContext.hpp"
#include "parallel/MpiGhostTransport.hpp"
#include "parallel/MpiPacketTransport.hpp"
#include "parallel/MpiSession.hpp"

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
