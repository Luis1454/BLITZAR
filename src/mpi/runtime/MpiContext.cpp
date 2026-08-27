#include "mpi/runtime/MpiState.hpp"

#include <new>

namespace blitzar_parallel {

MpiContext::MpiContext() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        status_ = BLITZAR_STATUS_ALLOCATION_FAILURE;

        return;
    }

    status_ = impl_->session.Status();
}

MpiContext::~MpiContext() noexcept = default;

bool MpiContext::IsUsable() const noexcept
{
    return impl_ != nullptr && status_ == BLITZAR_STATUS_OK;
}

bool MpiContext::IsDistributed() const noexcept
{
    return IsUsable() && impl_->session.IsDistributed();
}

int MpiContext::Rank() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->session.Rank();
}

int MpiContext::Size() const noexcept
{
    return impl_ == nullptr ? 1 : impl_->session.Size();
}

blitzar_status MpiContext::Status() const noexcept
{
    return status_;
}

} // namespace blitzar_parallel
