#include "parallel/mpi/exchange/ghost/MpiGhostTransport.hpp"

#include "parallel/mpi/exchange/ghost/MpiGhostState.hpp"

#include <memory>
#include <new>
#include <stdexcept>

namespace blitzar_parallel {

MpiGhostTransport::MpiGhostTransport(const MpiSession& session, const MpiCollectives& collectives,
    const MpiPacketTransport& packets) noexcept
    : session_(session), collectives_(collectives), packets_(packets)
{
}

blitzar_status MpiGhostTransport::Prepare(MpiGhostExchange& exchange, std::size_t send_capacity,
    std::size_t receive_capacity) const noexcept
{
    if (!session_.IsUsable()) {
        return session_.Status();
    }
    if (!session_.IsDistributed()) {
        exchange.impl_.reset();

        return BLITZAR_STATUS_OK;
    }
    if (exchange.impl_ != nullptr && exchange.impl_->active) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        if (exchange.impl_ == nullptr) {
            exchange.impl_ = std::make_unique<MpiGhostExchange::Impl>();
        }

        MpiGhostExchange::Impl& state = *exchange.impl_;
        state.transfer = {};

        return PrepareStorage(state, send_capacity, receive_capacity);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

bool MpiGhostTransport::IsActive(const MpiGhostExchange& exchange) const noexcept
{
    return exchange.impl_ != nullptr && exchange.impl_->active;
}

void MpiGhostTransport::Abort(MpiGhostExchange& exchange) const noexcept
{
    if (exchange.impl_ == nullptr) {
        return;
    }

    AbortExchange(*exchange.impl_);
}

} // namespace blitzar_parallel
