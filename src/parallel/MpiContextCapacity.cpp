#include "parallel/MpiContextState.hpp"

namespace blitzar_parallel {

blitzar_status MpiContext::PrepareCapacity(
    std::size_t packet_capacity, GhostExchange& exchange) const noexcept
{
    return PrepareCapacity(packet_capacity, exchange, GhostCapacity{0, 0});
}

blitzar_status MpiContext::PrepareCapacity(std::size_t packet_capacity, GhostExchange& exchange,
    GhostCapacity ghost_capacity) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }

    const blitzar_status packet_status = impl_->packets.Prepare(packet_capacity);

    if (packet_status != BLITZAR_STATUS_OK) {
        return packet_status;
    }

    const std::size_t send_capacity =
        ghost_capacity.send == 0 ? packet_capacity : ghost_capacity.send;

    const std::size_t receive_capacity =
        ghost_capacity.receive == 0 ? packet_capacity : ghost_capacity.receive;

    return impl_->ghosts.Prepare(exchange, send_capacity, receive_capacity);
}

} // namespace blitzar_parallel
