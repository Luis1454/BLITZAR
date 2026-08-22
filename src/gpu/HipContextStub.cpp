#include "gpu/HipContext.hpp"

namespace blitzar_gpu {

struct HipContext::Impl final {
};

HipContext::HipContext() noexcept = default;

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

blitzar_status HipContext::ComputeDirect(
    blitzar_core::ParticleStateView,
    blitzar_core::ForceView,
    blitzar_physics::GravityParameters) noexcept
{
    return BLITZAR_STATUS_UNSUPPORTED;
}

blitzar_status HipContext::ComputeBarnesHut(
    blitzar_core::ParticleStateView,
    blitzar_core::ForceView,
    const blitzar_core::ExecutionSettings&,
    blitzar_physics::GravityParameters,
    blitzar_barnes_hut::BarnesHutSettings) noexcept
{
    return BLITZAR_STATUS_UNSUPPORTED;
}

}  // namespace blitzar_gpu
