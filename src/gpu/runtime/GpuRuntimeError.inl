namespace blitzar_hip {

namespace {

[[nodiscard]] blitzar_status ClearDeviceError(GpuBuffers& buffers) noexcept
{
    return hipMemsetAsync(reinterpret_cast<void*>(buffers.DeviceError()), 0, sizeof(int),
               reinterpret_cast<hipStream_t>(buffers.Stream())) == hipSuccess
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}

[[nodiscard]] blitzar_status QueueDeviceError(GpuBuffers& buffers) noexcept
{
    return BlitzarMemcpyAsync({reinterpret_cast<void*>(buffers.HostError()),
               reinterpret_cast<const void*>(buffers.DeviceError()), sizeof(int),
               hipMemcpyDeviceToHost, reinterpret_cast<hipStream_t>(buffers.Stream())}) ==
                   hipSuccess
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INTERNAL_ERROR;
}

} // namespace

} // namespace blitzar_hip
