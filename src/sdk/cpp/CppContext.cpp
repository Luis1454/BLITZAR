#include "sdk/cpp/CppState.hpp"

#include <new>
#include <utility>

namespace blitzar {

Context::Context() noexcept : impl_(nullptr), status_(Status::InvalidArgument)
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        status_ = Status::AllocationFailure;

        return;
    }

    blitzar_context* context = nullptr;

    const blitzar_status status = blitzar_context_create(&context);

    impl_->handle.reset(context);

    status_ = FromCStatus(status);
}

Context::~Context() noexcept = default;

Context::Context(Context&& other) noexcept : impl_(std::move(other.impl_)), status_(other.status_)
{
    other.status_ = Status::InvalidArgument;
}

Context& Context::operator=(Context&& other) noexcept
{
    if (this != &other) {
        impl_ = std::move(other.impl_);
        status_ = other.status_;
        other.status_ = Status::InvalidArgument;
    }

    return *this;
}

bool Context::valid() const noexcept
{
    return impl_ != nullptr && impl_->handle != nullptr && status_ == Status::Ok;
}

Status Context::status() const noexcept
{
    return status_;
}

} // namespace blitzar
