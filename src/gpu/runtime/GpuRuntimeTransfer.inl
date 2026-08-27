namespace blitzar_hip {

namespace {

template <std::size_t Count>
[[nodiscard]] blitzar_status UploadScalars(GpuBuffers& buffers,
    const std::array<std::span<const double>, Count>& source,
    const std::array<std::uintptr_t, Count>& host,
    const std::array<std::uintptr_t, Count>& device) noexcept
{
    for (std::size_t index = 0; index < Count; ++index) {
        std::copy(
            source[index].begin(), source[index].end(), reinterpret_cast<double*>(host[index]));

        const hipError_t error = BlitzarMemcpyAsync({reinterpret_cast<void*>(device[index]),
            reinterpret_cast<const void*>(host[index]), source[index].size_bytes(),
            hipMemcpyHostToDevice, reinterpret_cast<hipStream_t>(buffers.Stream())});

        if (error != hipSuccess) {
            buffers.Disable();

            return HipStatus(error);
        }
    }

    return BLITZAR_STATUS_OK;
}

} // namespace

blitzar_status GpuContext::Impl::EnsureBuffers(
    std::size_t target_count, std::size_t source_count, std::size_t cell_count) noexcept
{
    return buffers.Ensure(target_count, source_count, cell_count)
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_ALLOCATION_FAILURE;
}

blitzar_status GpuContext::Impl::UploadFullState(blitzar_core::ParticleStateView particles) noexcept
{
    const std::array<std::span<const double>, 4> source{
        particles.x, particles.y, particles.z, particles.mass};

    const std::array<std::uintptr_t, 4> host{buffers.HostParticle(0), buffers.HostParticle(1),
        buffers.HostParticle(2), buffers.HostParticle(3)};

    const std::array<std::uintptr_t, 4> device{buffers.DeviceParticle(0), buffers.DeviceParticle(1),
        buffers.DeviceParticle(2), buffers.DeviceParticle(3)};

    return UploadScalars(buffers, source, host, device);
}

blitzar_status GpuContext::Impl::UploadTargetState(
    blitzar_core::ParticleStateView particles) noexcept
{
    const std::array<std::span<const double>, 4> source{particles.x.first(particles.count),
        particles.y.first(particles.count), particles.z.first(particles.count),
        particles.mass.first(particles.count)};

    const std::array<std::uintptr_t, 4> host{buffers.HostParticle(0), buffers.HostParticle(1),
        buffers.HostParticle(2), buffers.HostParticle(3)};

    const std::array<std::uintptr_t, 4> device{buffers.DeviceParticle(0), buffers.DeviceParticle(1),
        buffers.DeviceParticle(2), buffers.DeviceParticle(3)};

    return UploadScalars(buffers, source, host, device);
}

blitzar_status GpuContext::Impl::UploadSourceState(
    blitzar_core::ParticleStateView particles, blitzar_solvers::ForceRange range) noexcept
{
    const std::size_t source_count = range.source_end - range.source_begin;
    const std::array<std::span<const double>, 4> source{
        particles.x.subspan(range.source_begin, source_count),
        particles.y.subspan(range.source_begin, source_count),
        particles.z.subspan(range.source_begin, source_count),
        particles.mass.subspan(range.source_begin, source_count)};

    const std::array<std::uintptr_t, 4> host{
        buffers.HostSource(0), buffers.HostSource(1), buffers.HostSource(2), buffers.HostSource(3)};

    const std::array<std::uintptr_t, 4> device{buffers.DeviceSource(0), buffers.DeviceSource(1),
        buffers.DeviceSource(2), buffers.DeviceSource(3)};

    return UploadScalars(buffers, source, host, device);
}

blitzar_status GpuContext::Impl::UploadForces(blitzar_core::ForceView forces) noexcept
{
    const std::array<std::span<const double>, 3> source{forces.x, forces.y, forces.z};
    const std::array<std::uintptr_t, 3> host{
        buffers.HostForce(0), buffers.HostForce(1), buffers.HostForce(2)};

    const std::array<std::uintptr_t, 3> device{
        buffers.DeviceForce(0), buffers.DeviceForce(1), buffers.DeviceForce(2)};

    return UploadScalars(buffers, source, host, device);
}

blitzar_status GpuContext::Impl::QueueForceDownloads(std::size_t particle_count) noexcept
{
    const std::size_t bytes = particle_count * sizeof(double);

    for (std::size_t index = 0; index < 3; ++index) {
        const hipError_t error =
            BlitzarMemcpyAsync({reinterpret_cast<void*>(buffers.HostForce(index)),
                reinterpret_cast<const void*>(buffers.DeviceForce(index)), bytes,
                hipMemcpyDeviceToHost, reinterpret_cast<hipStream_t>(buffers.Stream())});

        if (error != hipSuccess) {
            buffers.Disable();

            return HipStatus(error);
        }
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status GpuContext::Impl::Finish(
    std::size_t particle_count, blitzar_core::ForceView forces) noexcept
{
    const hipError_t synchronization =
        hipStreamSynchronize(reinterpret_cast<hipStream_t>(buffers.Stream()));

    if (synchronization != hipSuccess) {
        buffers.Disable();

        return HipStatus(synchronization);
    }

    const int device_status = *reinterpret_cast<const int*>(buffers.HostError());

    if (device_status != 0) {
        return static_cast<blitzar_status>(device_status);
    }

    const std::array<std::span<double>, 3> destination{forces.x, forces.y, forces.z};

    for (std::size_t index = 0; index < destination.size(); ++index) {
        std::copy_n(reinterpret_cast<const double*>(buffers.HostForce(index)), particle_count,
            destination[index].begin());
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_hip
