#include "accelerators/gpu/hip/runtime/Context.hpp"

#include <memory>
#include <new>

namespace blitzar_hip {

struct Context::Impl final {
    Fault fault{Fault::None};
};

namespace {

[[nodiscard]] blitzar_status FaultStatus(Fault fault) noexcept
{
    switch (fault) {
    case Fault::None:

        return BLITZAR_STATUS_OK;

    case Fault::AllocationFailure:

        return BLITZAR_STATUS_ALLOCATION_FAILURE;

    case Fault::LaunchFailure:
    case Fault::SynchronizationFailure:

        return BLITZAR_STATUS_INTERNAL_ERROR;

    case Fault::NonFiniteResult:

        return BLITZAR_STATUS_INVALID_ARGUMENT;

    default:

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
}

} // namespace

Context::Context() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        impl_.reset();

        status_ = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

Context::~Context() noexcept = default;

Context::Context(Context&& other) noexcept = default;

Context& Context::operator=(Context&& other) noexcept = default;

bool Context::IsCompiled() const noexcept
{
    return false;
}

bool Context::IsAvailable() const noexcept
{
    return false;
}

void Context::SetFaultForTesting(Fault fault) noexcept
{
    if (impl_ != nullptr) {
        impl_->fault = fault;
    }
}

blitzar_status Context::ComputeDirect(blitzar_core::ParticleStateView, blitzar_core::ForceView,
    blitzar_physics::GravityParameters) noexcept
{
    if (status_ != BLITZAR_STATUS_OK) {
        return status_;
    }
    if (impl_ != nullptr) {
        const blitzar_status fault_status = FaultStatus(impl_->fault);

        if (fault_status != BLITZAR_STATUS_OK) {
            return fault_status;
        }
    }

    return BLITZAR_STATUS_UNSUPPORTED;
}

blitzar_status Context::ComputeDirectRange(blitzar_core::ParticleStateView, blitzar_core::ForceView,
    blitzar_physics::GravityParameters, blitzar_solvers::ForceRange) noexcept
{
    if (status_ != BLITZAR_STATUS_OK) {
        return status_;
    }
    if (impl_ != nullptr) {
        const blitzar_status fault_status = FaultStatus(impl_->fault);

        if (fault_status != BLITZAR_STATUS_OK) {
            return fault_status;
        }
    }

    return BLITZAR_STATUS_UNSUPPORTED;
}

blitzar_status Context::ComputeBarnesHut(const BarnesHutComputeRequest&) noexcept
{
    if (status_ != BLITZAR_STATUS_OK) {
        return status_;
    }
    if (impl_ != nullptr) {
        const blitzar_status fault_status = FaultStatus(impl_->fault);

        if (fault_status != BLITZAR_STATUS_OK) {
            return fault_status;
        }
    }

    return BLITZAR_STATUS_UNSUPPORTED;
}

} // namespace blitzar_hip
