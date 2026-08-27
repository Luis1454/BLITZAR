#include "mpi/packets/MpiPacketTransport.hpp"

namespace blitzar_parallel {

MpiPacketTransport::MpiPacketTransport(
    const MpiNativeSession& session, const MpiCollectives& collectives) noexcept
    : session_(session), collectives_(collectives)
{
}

} // namespace blitzar_parallel
