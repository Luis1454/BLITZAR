#include "mpi/native/MpiNativeSession.hpp"

#include <memory>
#include <new>

namespace blitzar_parallel {

struct MpiNativeSession::Impl final {
    MpiNative native;
};

MpiNativeSession::MpiNativeSession() noexcept
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

MpiNativeSession::~MpiNativeSession() noexcept = default;

bool MpiNativeSession::IsUsable() const noexcept
{
    return status_ == BLITZAR_STATUS_OK;
}

bool MpiNativeSession::IsDistributed() const noexcept
{
    return IsUsable() && size_ > 1;
}

int MpiNativeSession::Rank() const noexcept
{
    return rank_;
}

int MpiNativeSession::Size() const noexcept
{
    return size_;
}

blitzar_status MpiNativeSession::Status() const noexcept
{
    return status_;
}

const MpiNative& MpiNativeSession::Native() const noexcept
{
    return impl_->native;
}

} // namespace blitzar_parallel
