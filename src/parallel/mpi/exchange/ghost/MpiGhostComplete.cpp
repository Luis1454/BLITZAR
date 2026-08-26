#include "parallel/mpi/exchange/ghost/MpiGhostProtocol.hpp"
#include "parallel/mpi/exchange/ghost/MpiGhostState.hpp"
#include "parallel/mpi/exchange/ghost/MpiGhostTransport.hpp"

#include <cstddef>

namespace blitzar_parallel {

blitzar_status MpiGhostTransport::Complete(
    MpiGhostExchange& exchange, PacketBuffer& ghosts) const noexcept
{
    ghosts.Clear();

    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        return BLITZAR_STATUS_OK;
    }

    const bool exchange_active = exchange.impl_ != nullptr && exchange.impl_->active;
    blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    const blitzar_status synchronization_status = collectives_.SynchronizeStatus(
        exchange_active ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INVALID_ARGUMENT, "MpiGhostTransport",
        "ghost-complete-preflight", global_status);

    if (synchronization_status != BLITZAR_STATUS_OK || global_status != BLITZAR_STATUS_OK) {
        Abort(exchange);

        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status : global_status;
    }
    if (exchange.impl_ == nullptr || !exchange.impl_->active) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return CompleteActive(*exchange.impl_, ghosts);
}

blitzar_status MpiGhostTransport::CompleteActive(
    MpiGhostExchange::Impl& state, PacketBuffer& ghosts) const noexcept
{
    blitzar_status status = WaitGhostRequests(state);

    if (status != BLITZAR_STATUS_OK) {
        AbortExchange(state);

        return status;
    }

    std::size_t total = 0;

    status = CountReceived(state, total);

    blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    blitzar_status synchronization_status = collectives_.SynchronizeStatus(
        status, "MpiGhostTransport", "ghost-count-prepare", global_status);

    if (synchronization_status != BLITZAR_STATUS_OK || global_status != BLITZAR_STATUS_OK) {
        AbortExchange(state);

        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status : global_status;
    }

    status = PrepareGhostBuffer(total, ghosts);
    global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    synchronization_status = collectives_.SynchronizeStatus(
        status, "MpiGhostTransport", "ghost-data-prepare", global_status);

    if (synchronization_status != BLITZAR_STATUS_OK || global_status != BLITZAR_STATUS_OK) {
        AbortExchange(state);

        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status : global_status;
    }

    status = DecodeReceived(state, ghosts);
    global_status = BLITZAR_STATUS_INTERNAL_ERROR;
    synchronization_status = collectives_.SynchronizeStatus(
        status, "MpiGhostTransport", "ghost-data-decode", global_status);

    ClearExchange(state);

    return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status : global_status;
}

blitzar_status MpiGhostTransport::WaitGhostRequests(MpiGhostExchange::Impl& state) const noexcept
{
    return state.native == nullptr
               ? BLITZAR_STATUS_INVALID_ARGUMENT
               : session_.Native().WaitGhost(*state.native, state.receive_byte_counts);
}

} // namespace blitzar_parallel
