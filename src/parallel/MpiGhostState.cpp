#include "parallel/MpiGhostState.hpp"

#include "parallel/MpiGhostTransport.hpp"

#include <utility>

namespace blitzar_parallel {

void MpiGhostTransport::ClearExchange(MpiGhostExchange::Impl& state) noexcept
{
    state.active = false;

    state.local_wire.clear();
    state.receive_wire.clear();
    state.receive_byte_counts.clear();
    state.receive_chunks.clear();

    if (state.native != nullptr) {
        state.native->Reset();
    }
}

void MpiGhostTransport::AbortExchange(MpiGhostExchange::Impl& state) noexcept
{
    if (state.active && state.native != nullptr) {
        state.native->Cancel();
    }

    ClearExchange(state);

    state.transfer = {};
}

MpiGhostExchange::MpiGhostExchange() noexcept = default;

MpiGhostExchange::~MpiGhostExchange() noexcept
{
    if (impl_ != nullptr) {
        MpiGhostTransport::AbortExchange(*impl_);
    }
}

MpiGhostExchange::MpiGhostExchange(MpiGhostExchange&& other) noexcept = default;

MpiGhostExchange& MpiGhostExchange::operator=(MpiGhostExchange&& other) noexcept
{
    if (this == &other) {
        return *this;
    }

    if (impl_ != nullptr) {
        MpiGhostTransport::AbortExchange(*impl_);
    }

    impl_ = std::move(other.impl_);

    return *this;
}

MpiGhostExchange::TransferStats MpiGhostExchange::Transfer() const noexcept
{
    return impl_ == nullptr ? TransferStats{} : impl_->transfer;
}

} // namespace blitzar_parallel
