#include "accelerators/gpu/hip/launch/Launch.hpp"

namespace blitzar_hip {

blitzar_status Context::Impl::ComputeBarnesHut(const BarnesHutComputeRequest& request) noexcept
{
    const blitzar_core::ParticleStateView particles = request.particles;
    const blitzar_core::ForceView forces = request.forces;
    const blitzar_physics::GravityParameters gravity = request.gravity;
    const blitzar_barnes_hut::BarnesHutSettings settings = request.settings;
    blitzar_status status = PrepareTree(particles, settings);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    const std::span<const blitzar_trees::Octree::Cell> cells = tree->Cells();
    const std::span<const std::size_t> indices = tree->Indices();

    status = EnsureBuffers(particles.SourceCount(), 0, cells.size());

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = UploadTree(cells, indices);

    if (status != BLITZAR_STATUS_OK) {
        buffers.Disable();

        return status;
    }

    status = UploadFullState(particles);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    status = ClearDeviceError(buffers);

    if (status != BLITZAR_STATUS_OK) {
        buffers.Disable();

        return status;
    }

    const blitzar_accelerator_launch::DeviceParticleAddresses addresses{buffers.DeviceParticle(0),
        buffers.DeviceParticle(1), buffers.DeviceParticle(2), buffers.DeviceParticle(3),
        buffers.DeviceForce(0), buffers.DeviceForce(1), buffers.DeviceForce(2)};

    const blitzar_accelerator_launch::BarnesHutLaunchRequest launch_request{addresses,
        particles.count, particles.SourceCount(), settings.opening_angle,
        {buffers.DeviceCells(), cells.size(), buffers.DeviceIndices()},

        {gravity.EffectiveConstant(), gravity.EffectiveSoftening()}, settings.max_depth,

        {buffers.DeviceError(), buffers.Stream()}};

    status = blitzar_accelerator_launch::LaunchBarnesHut(launch_request);

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

} // namespace blitzar_hip
