#include "parallel/MpiContextState.hpp"

namespace blitzar_parallel {

blitzar_status MpiContext::SynchronizeStatus(blitzar_status local_status, const char* operation,
    const char* phase, blitzar_status& global_status) const noexcept
{
    if (impl_ == nullptr) {
        global_status = status_;

        return status_;
    }

    return impl_->collectives.SynchronizeStatus(local_status, operation, phase, global_status);
}

blitzar_status MpiContext::ReduceBounds(
    std::span<blitzar_core::Scalar> minimum, std::span<blitzar_core::Scalar> maximum) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }

    return impl_->collectives.ReduceBounds(minimum, maximum);
}

blitzar_status MpiContext::ReduceMax(int local_value, int& global_value) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }

    return impl_->collectives.ReduceMax(local_value, global_value);
}

blitzar_status MpiContext::Broadcast(
    std::span<blitzar_core::Scalar> values, int root) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }

    return impl_->collectives.Broadcast(values, root);
}

blitzar_status MpiContext::Broadcast(std::span<std::uint64_t> values, int root) const noexcept
{
    if (impl_ == nullptr) {
        return status_;
    }

    return impl_->collectives.Broadcast(values, root);
}

} // namespace blitzar_parallel
