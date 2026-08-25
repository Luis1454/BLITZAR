#include "solvers/gpu/HipKernel.hpp"

namespace blitzar_gpu {

namespace {

[[nodiscard]] bool IsSourceAlias(
    blitzar_core::ParticleStateView particles, blitzar_core::ForceRange range) noexcept
{
    const std::size_t source_count = range.source_end - range.source_begin;

    return range.source_begin == 0 && source_count == particles.count &&
           particles.SourceCount() == particles.count;
}

} // namespace

blitzar_status HipContext::Impl::UploadDirectInputs(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, blitzar_core::ForceRange range,
    bool source_alias_target) noexcept
{
    const std::size_t source_count = range.source_end - range.source_begin;
    blitzar_status status = UploadTargetState(particles);

    if (status == BLITZAR_STATUS_OK && !source_alias_target && source_count != 0) {
        status = UploadSourceState(particles, range);
    }
    if (status == BLITZAR_STATUS_OK && range.accumulate) {
        status = UploadForces(forces);
    }

    return status;
}

blitzar_status HipContext::Impl::ComputeDirect(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, blitzar_physics::GravityParameters gravity,
    blitzar_core::ForceRange range) noexcept
{
    const std::size_t source_count = range.source_end - range.source_begin;
    const bool source_alias_target = IsSourceAlias(particles, range);
    blitzar_status status =
        EnsureBuffers(particles.count, source_alias_target ? 0 : source_count, 0);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = UploadDirectInputs(particles, forces, range, source_alias_target);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = ClearDeviceError(buffers);

    if (status != BLITZAR_STATUS_OK) {
        buffers.Disable();

        return status;
    }

    const blitzar_gpu_detail::DeviceParticleAddresses addresses{buffers.DeviceParticle(0),
        buffers.DeviceParticle(1), buffers.DeviceParticle(2), buffers.DeviceParticle(3),
        buffers.DeviceForce(0), buffers.DeviceForce(1), buffers.DeviceForce(2),
        source_alias_target ? buffers.DeviceParticle(0) : buffers.DeviceSource(0),
        source_alias_target ? buffers.DeviceParticle(1) : buffers.DeviceSource(1),
        source_alias_target ? buffers.DeviceParticle(2) : buffers.DeviceSource(2),
        source_alias_target ? buffers.DeviceParticle(3) : buffers.DeviceSource(3)};

    const blitzar_gpu_detail::DirectLaunchRequest launch_request{addresses, particles.count,

        {0, source_count, range.accumulate},

        {gravity.EffectiveConstant(), gravity.EffectiveSoftening()},

        {buffers.DeviceError(), buffers.Stream()}, range.source_begin};

    status = blitzar_gpu_detail::LaunchDirect(launch_request);

    if (status != BLITZAR_STATUS_OK) {
        buffers.Disable();

        return status;
    }

    status = QueueDeviceError(buffers);

    if (status != BLITZAR_STATUS_OK) {
        buffers.Disable();

        return status;
    }

    status = QueueForceDownloads(particles.count);

    return status == BLITZAR_STATUS_OK ? Finish(particles.count, forces) : status;
}

} // namespace blitzar_gpu
