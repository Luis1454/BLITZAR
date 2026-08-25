#include "parallel/MpiSessionNative.hpp"

#include <memory>
#include <new>

namespace blitzar_parallel {

MpiSession::MpiSession() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        status_ = BLITZAR_STATUS_ALLOCATION_FAILURE;

        return;
    }

    status_ = InitializeMpi();
}

MpiSession::~MpiSession() noexcept
{
#if defined(BLITZAR_HAS_MPI)
    if (impl_ == nullptr) {
        return;
    }

    const auto& runtime = *impl_;

    if (!runtime.registered) {
        return;
    }

    ReleaseMpi();
#endif
}

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

const MpiSession::Impl& MpiSession::Native() const noexcept
{
    return *impl_;
}

} // namespace blitzar_parallel
