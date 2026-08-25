namespace blitzar_gpu_detail {

namespace {

[[nodiscard]] bool HasTargetAddresses(const DeviceParticleAddresses& addresses) noexcept
{
    return addresses.position_x != 0 && addresses.position_y != 0 &&
           addresses.position_z != 0 && addresses.mass != 0 && addresses.force_x != 0 &&
           addresses.force_y != 0 && addresses.force_z != 0;
}

[[nodiscard]] bool HasSourceAddresses(const DeviceParticleAddresses& addresses) noexcept
{
    return addresses.source_position_x != 0 && addresses.source_position_y != 0 &&
           addresses.source_position_z != 0 && addresses.source_mass != 0;
}

[[nodiscard]] blitzar_status ValidateLaunch(const DirectLaunchRequest& request) noexcept
{
    const std::size_t target_count = request.target_count;
    const blitzar_core::ForceRange& range = request.range;

    if (target_count == 0) {
        return BLITZAR_STATUS_OK;
    }
    if (request.runtime.error_address == 0 || request.runtime.stream == 0 ||
        range.source_begin > range.source_end || !HasTargetAddresses(request.addresses)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (range.source_begin != range.source_end && !HasSourceAddresses(request.addresses)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (target_count > std::numeric_limits<std::size_t>::max() - (BlockSize - 1)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t block_count = (target_count + BlockSize - 1) / BlockSize;

    return block_count > std::numeric_limits<unsigned int>::max()
               ? BLITZAR_STATUS_INVALID_ARGUMENT
               : BLITZAR_STATUS_OK;
}

} // namespace

blitzar_status LaunchDirect(const DirectLaunchRequest& request) noexcept
{
    const blitzar_status validation_status = ValidateLaunch(request);

    if (validation_status != BLITZAR_STATUS_OK || request.target_count == 0) {
        return validation_status;
    }

    const std::size_t block_count = (request.target_count + BlockSize - 1) / BlockSize;

    const DeviceParticleAddresses& addresses = request.addresses;

    const blitzar_core::ForceRange& range = request.range;

    const KernelPhysics& physics = request.physics;
    const KernelRuntime& runtime = request.runtime;

    const bool source_alias_target =
        range.source_begin == 0 && range.source_end == request.target_count &&
        addresses.source_position_x == addresses.position_x &&
        addresses.source_position_y == addresses.position_y &&
        addresses.source_position_z == addresses.position_z &&
        addresses.source_mass == addresses.mass;

    const DirectDeviceRequest kernel_request{
        reinterpret_cast<const double*>(addresses.position_x),
        reinterpret_cast<const double*>(addresses.position_y),
        reinterpret_cast<const double*>(addresses.position_z),
        reinterpret_cast<const double*>(addresses.mass),
        reinterpret_cast<double*>(addresses.force_x), reinterpret_cast<double*>(addresses.force_y),
        reinterpret_cast<double*>(addresses.force_z),
        reinterpret_cast<const double*>(source_alias_target ? addresses.position_x
                                                            : addresses.source_position_x),
        reinterpret_cast<const double*>(source_alias_target ? addresses.position_y
                                                            : addresses.source_position_y),
        reinterpret_cast<const double*>(source_alias_target ? addresses.position_z
                                                            : addresses.source_position_z),
        reinterpret_cast<const double*>(source_alias_target ? addresses.mass
                                                            : addresses.source_mass),
        request.target_count, range.source_begin, range.source_end,
        request.source_global_begin, physics.gravitational_constant, physics.softening,
        range.accumulate, reinterpret_cast<int*>(runtime.error_address)};

    hipLaunchKernelGGL(DirectKernel, dim3(static_cast<unsigned int>(block_count)), dim3(BlockSize),
        4 * BlockSize * sizeof(double), reinterpret_cast<hipStream_t>(runtime.stream),
        kernel_request);

    return hipGetLastError() == hipSuccess ? BLITZAR_STATUS_OK : BLITZAR_STATUS_INTERNAL_ERROR;
}

} // namespace blitzar_gpu_detail
