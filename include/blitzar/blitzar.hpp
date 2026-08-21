#ifndef BLITZAR_BLITZAR_HPP
#define BLITZAR_BLITZAR_HPP

#include <blitzar/blitzar.h>

#include <utility>

namespace blitzar {

enum class Status : int {
    Ok = BLITZAR_STATUS_OK,
    InvalidArgument = BLITZAR_STATUS_INVALID_ARGUMENT,
    AllocationFailure = BLITZAR_STATUS_ALLOCATION_FAILURE,
    InternalError = BLITZAR_STATUS_INTERNAL_ERROR,
    Singularity = BLITZAR_STATUS_SINGULARITY
};

class BLITZAR_API Context final {
public:
    Context() noexcept;
    ~Context() noexcept;

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    Context(Context&& other) noexcept;
    Context& operator=(Context&& other) noexcept;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] Status status() const noexcept;

private:
    explicit Context(blitzar_context* context, Status status) noexcept;

    blitzar_context* context_;
    Status status_;
};

}  // namespace blitzar

#endif
