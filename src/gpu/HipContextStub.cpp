#include "gpu/HipContext.hpp"

#include <memory>
#include <new>

namespace blitzar_gpu {

struct HipContext::Impl final {
    HipFault fault{HipFault::None};
};

namespace {

[[nodiscard]] blitzar_status FaultStatus(HipFault fault) noexcept
{
    switch (fault) {
    case HipFault::None:

        return BLITZAR_STATUS_OK;

    case HipFault::AllocationFailure:

        return BLITZAR_STATUS_ALLOCATION_FAILURE;

    case HipFault::LaunchFailure:
    case HipFault::SynchronizationFailure:

        return BLITZAR_STATUS_INTERNAL_ERROR;

    case HipFault::NonFiniteResult:

        return BLITZAR_STATUS_INVALID_ARGUMENT;

    default:

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
}

} // namespace

HipContext::HipContext() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        impl_.reset();
        status_ = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

HipContext::~HipContext() noexcept = default;

HipContext::HipContext(HipContext&& other) noexcept = default;

HipContext& HipContext::operator=(HipContext&& other) noexcept = default;

bool HipContext::IsCompiled() const noexcept
{
    return false;
}

bool HipContext::IsAvailable() const noexcept
{
    return false;
}

void HipContext::SetFaultForTesting(HipFault fault) noexcept
{
    if (impl_ != nullptr) {
        impl_->fault = fault;
    }
}

blitzar_status HipContext::ComputeDirect(blitzar_core::ParticleStateView, blitzar_core::ForceView,
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

blitzar_status HipContext::ComputeDirectRange(blitzar_core::ParticleStateView,
    blitzar_core::ForceView, blitzar_physics::GravityParameters, blitzar_core::ForceRange) noexcept
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

blitzar_status HipContext::ComputeBarnesHut(blitzar_core::ParticleStateView,
    blitzar_core::ForceView, const blitzar_core::ExecutionSettings&,
    blitzar_physics::GravityParameters, blitzar_barnes_hut::BarnesHutSettings) noexcept
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

} // namespace blitzar_gpu
