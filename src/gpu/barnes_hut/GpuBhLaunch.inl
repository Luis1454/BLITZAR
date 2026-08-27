namespace blitzar_accelerator_launch {

namespace {

[[nodiscard]] bool HasParticleAddresses(const DeviceParticleAddresses& addresses) noexcept
{
    return addresses.position_x != 0 && addresses.position_y != 0 && addresses.position_z != 0 &&
           addresses.mass != 0 && addresses.force_x != 0 && addresses.force_y != 0 &&
           addresses.force_z != 0;
}

[[nodiscard]] blitzar_status ValidateLaunch(const BarnesHutLaunchRequest& request) noexcept
{
    if (request.target_count == 0) {
        return BLITZAR_STATUS_OK;
    }
    if (request.tree.cells == 0 || request.tree.indices == 0 || request.tree.cell_count == 0 ||
        request.runtime.error_address == 0 || request.runtime.stream == 0 ||
        !HasParticleAddresses(request.addresses)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (request.max_depth > 36) {
        return BLITZAR_STATUS_UNSUPPORTED;
    }
    if (request.target_count > std::numeric_limits<std::size_t>::max() - (BlockSize - 1)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t block_count = (request.target_count + BlockSize - 1) / BlockSize;

    return block_count > std::numeric_limits<unsigned int>::max() ? BLITZAR_STATUS_INVALID_ARGUMENT
                                                                  : BLITZAR_STATUS_OK;
}

} // namespace

blitzar_status LaunchBarnesHut(const BarnesHutLaunchRequest& request) noexcept
{
    const blitzar_status validation_status = ValidateLaunch(request);

    if (validation_status != BLITZAR_STATUS_OK || request.target_count == 0) {
        return validation_status;
    }

    const std::size_t block_count = (request.target_count + BlockSize - 1) / BlockSize;

    const DeviceParticleAddresses& addresses = request.addresses;
    const TreeAddresses& tree = request.tree;
    const KernelPhysics& physics = request.physics;
    const KernelRuntime& runtime = request.runtime;

    const BarnesHutDeviceRequest kernel_request{
        reinterpret_cast<const double*>(addresses.position_x),
        reinterpret_cast<const double*>(addresses.position_y),
        reinterpret_cast<const double*>(addresses.position_z),
        reinterpret_cast<const double*>(addresses.mass),
        reinterpret_cast<double*>(addresses.force_x), reinterpret_cast<double*>(addresses.force_y),
        reinterpret_cast<double*>(addresses.force_z), request.target_count, request.source_count,
        reinterpret_cast<const GpuCell*>(tree.cells), tree.cell_count,
        reinterpret_cast<const std::uint64_t*>(tree.indices), request.opening_angle,
        physics.gravitational_constant, physics.softening,
        reinterpret_cast<int*>(runtime.error_address)};

    hipLaunchKernelGGL(BarnesHutKernel, dim3(static_cast<unsigned int>(block_count)),
        dim3(BlockSize), 0, reinterpret_cast<hipStream_t>(runtime.stream), kernel_request);

    return hipGetLastError() == hipSuccess ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR;
}

} // namespace blitzar_accelerator_launch
