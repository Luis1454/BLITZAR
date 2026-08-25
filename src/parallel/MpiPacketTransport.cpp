#include "parallel/MpiPacketTransport.hpp"

namespace blitzar_parallel {

MpiPacketTransport::MpiPacketTransport(
    const MpiSession& session, const MpiCollectives& collectives) noexcept
    : session_(session), collectives_(collectives)
{
}

} // namespace blitzar_parallel
