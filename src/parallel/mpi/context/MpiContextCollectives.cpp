#include "parallel/mpi/context/MpiContextState.hpp"

namespace blitzar_parallel {

blitzar_status MpiContext::SynchronizeStatus(blitzar_status local_status,
    std::string_view operation, std::string_view phase,
    blitzar_status& global_status) const noexcept
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
