#include <blitzar/blitzar.hpp>

namespace blitzar {

Context::Context() noexcept
    : context_(nullptr), status_(Status::InvalidArgument)
{
    blitzar_context* context = nullptr;
    const blitzar_status status = blitzar_context_create(&context);
    context_ = context;
    status_ = static_cast<Status>(status);
}

Context::Context(blitzar_context* context, Status status) noexcept
    : context_(context), status_(status)
{
}

Context::~Context() noexcept
{
    blitzar_context_destroy(context_);
}

Context::Context(Context&& other) noexcept
    : context_(other.context_), status_(other.status_)
{
    other.context_ = nullptr;
    other.status_ = Status::InvalidArgument;
}

Context& Context::operator=(Context&& other) noexcept
{
    if (this != &other) {
        blitzar_context_destroy(context_);
        context_ = other.context_;
        status_ = other.status_;
        other.context_ = nullptr;
        other.status_ = Status::InvalidArgument;
    }
    return *this;
}

bool Context::valid() const noexcept
{
    return context_ != nullptr && status_ == Status::Ok;
}

Status Context::status() const noexcept
{
    return status_;
}

}  // namespace blitzar
