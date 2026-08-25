namespace blitzar_gpu {

namespace {

[[nodiscard]] bool ValidCapacity(
    std::size_t target_count, std::size_t source_count, std::size_t cell_count) noexcept
{
    const std::size_t maximum = std::numeric_limits<std::size_t>::max();

    return target_count <= maximum / sizeof(double) &&
           source_count <= maximum / sizeof(double) &&
           cell_count <= maximum / sizeof(blitzar_gpu_detail::GpuCell) &&
           target_count <= maximum / sizeof(std::uint64_t);
}

template <typename Allocation, std::size_t Count>
[[nodiscard]] bool ResizeAll(
    std::array<Allocation, Count>& buffers, std::size_t bytes) noexcept
{
    for (Allocation& buffer : buffers) {
        if (!buffer.Resize(bytes)) {
            return false;
        }
    }

    return true;
}

} // namespace

bool HipBuffers::Impl::Ensure(
    std::size_t target_count, std::size_t source_count, std::size_t cell_count) noexcept
{
    if (!ValidCapacity(target_count, source_count, cell_count)) {
        return false;
    }

    const std::size_t target_scalar_bytes = target_count * sizeof(double);
    const std::size_t source_scalar_bytes = source_count * sizeof(double);
    const std::size_t cell_bytes = cell_count * sizeof(blitzar_gpu_detail::GpuCell);
    const std::size_t index_bytes = target_count * sizeof(std::uint64_t);

    return ResizeAll(host_particles, target_scalar_bytes) &&
           ResizeAll(device_particles, target_scalar_bytes) &&
           ResizeAll(host_sources, source_scalar_bytes) &&
           ResizeAll(device_sources, source_scalar_bytes) &&
           ResizeAll(host_forces, target_scalar_bytes) &&
           ResizeAll(device_forces, target_scalar_bytes) &&
           host_error.Resize(sizeof(int)) && device_error.Resize(sizeof(int)) &&
           host_cells.Resize(cell_bytes) && device_cells.Resize(cell_bytes) &&
           host_indices.Resize(index_bytes) && device_indices.Resize(index_bytes);
}

} // namespace blitzar_gpu
