#include "parallel/MpiContextState.hpp"

namespace blitzar_parallel {

blitzar_status MpiContext::BeginGhostExchange(
    std::span<const ParticlePacket> local, GhostExchange& exchange) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }

    return impl_->ghosts.Begin(local, exchange);
}

blitzar_status MpiContext::CompleteGhostExchange(
    GhostExchange& exchange, PacketBuffer& ghosts) const noexcept
{
    if (impl_ == nullptr) {
        ghosts.Clear();

        return status_;
    }

    return impl_->ghosts.Complete(exchange, ghosts);
}

bool MpiContext::IsGhostExchangeActive(const GhostExchange& exchange) const noexcept
{
    return impl_ != nullptr && impl_->ghosts.IsActive(exchange);
}

void MpiContext::AbortGhostExchange(GhostExchange& exchange) const noexcept
{
    if (impl_ != nullptr) {
        impl_->ghosts.Abort(exchange);
    }
}

} // namespace blitzar_parallel
