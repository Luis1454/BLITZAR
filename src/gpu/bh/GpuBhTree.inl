#include "gpu/runtime/GpuLaunch.hpp"

namespace blitzar_hip {

blitzar_status GpuContext::Impl::EnsureTree(blitzar_barnes_hut::BarnesHutSettings settings) noexcept
{
    if (tree != nullptr && tree_ready && SameSettings(tree_settings, settings)) {
        return BLITZAR_STATUS_OK;
    }

    try {
        tree = std::make_unique<blitzar_trees::Octree>(
            settings.max_particles, settings.max_cells, settings.leaf_capacity, settings.max_depth);
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    tree_settings = settings;
    tree_ready = false;

    return BLITZAR_STATUS_OK;
}

blitzar_status GpuContext::Impl::PrepareTree(blitzar_core::ParticleStateView particles,
    blitzar_barnes_hut::BarnesHutSettings settings) noexcept
{
    blitzar_status status = EnsureTree(settings);

    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    if (!tree->Refit(particles)) {
        status = tree->Build(particles);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
    }

    tree_ready = true;

    return BLITZAR_STATUS_OK;
}

blitzar_status GpuContext::Impl::UploadTree(std::span<const blitzar_trees::Octree::Cell> cells,
    std::span<const std::size_t> indices) noexcept
{
    const std::span<blitzar_accelerator_launch::GpuCell> host_cell_data(
        reinterpret_cast<blitzar_accelerator_launch::GpuCell*>(buffers.HostCells()), cells.size());

    for (std::size_t index = 0; index < cells.size(); ++index) {
        const blitzar_trees::Octree::Cell& source = cells[index];

        blitzar_accelerator_launch::GpuCell& destination = host_cell_data[index];

        destination.center_x = source.center.x;
        destination.center_y = source.center.y;
        destination.center_z = source.center.z;
        destination.center_of_mass_x = source.center_of_mass.x;
        destination.center_of_mass_y = source.center_of_mass.y;
        destination.center_of_mass_z = source.center_of_mass.z;
        destination.half_extent = source.half_extent;
        destination.mass = source.mass;
        destination.begin = static_cast<std::uint64_t>(source.begin);
        destination.count = static_cast<std::uint64_t>(source.count);

        for (std::size_t child = 0; child < source.children.size(); ++child) {
            destination.children[child] =
                source.children[child] == blitzar_trees::Octree::Cell::InvalidIndex
                    ? std::numeric_limits<std::uint64_t>::max()
                    : static_cast<std::uint64_t>(source.children[child]);
        }
    }

    const std::span<std::uint64_t> host_index_data(
        reinterpret_cast<std::uint64_t*>(buffers.HostIndices()), indices.size());

    for (std::size_t index = 0; index < indices.size(); ++index) {
        host_index_data[index] = static_cast<std::uint64_t>(indices[index]);
    }

    const hipStream_t stream = reinterpret_cast<hipStream_t>(buffers.Stream());
    const hipError_t cells_status = BlitzarMemcpyAsync({reinterpret_cast<void*>(
                                                            buffers.DeviceCells()),
        reinterpret_cast<const void*>(buffers.HostCells()),
        cells.size() * sizeof(blitzar_accelerator_launch::GpuCell), hipMemcpyHostToDevice, stream});

    const hipError_t indices_status =
        BlitzarMemcpyAsync({reinterpret_cast<void*>(buffers.DeviceIndices()),
            reinterpret_cast<const void*>(buffers.HostIndices()),
            indices.size() * sizeof(std::uint64_t), hipMemcpyHostToDevice, stream});

    return cells_status == hipSuccess && indices_status == hipSuccess
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}

} // namespace blitzar_hip
