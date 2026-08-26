#include "parallel/mpi/native/MpiSession.hpp"

#include <memory>
#include <new>

namespace blitzar_parallel {

struct MpiSession::Impl final {
    MpiNative native;
};

MpiSession::MpiSession() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        status_ = BLITZAR_STATUS_ALLOCATION_FAILURE;

        return;
    }

    status_ = impl_->native.Status();
    rank_ = impl_->native.Rank();
    size_ = impl_->native.Size();
}

MpiSession::~MpiSession() noexcept = default;

bool MpiSession::IsUsable() const noexcept
{
    return status_ == BLITZAR_STATUS_OK;
}

bool MpiSession::IsDistributed() const noexcept
{
    return IsUsable() && size_ > 1;
}

int MpiSession::Rank() const noexcept
{
    return rank_;
}

int MpiSession::Size() const noexcept
{
    return size_;
}

blitzar_status MpiSession::Status() const noexcept
{
    return status_;
}

const MpiNative& MpiSession::Native() const noexcept
{
    return impl_->native;
}

} // namespace blitzar_parallel
