__device__ int BroadcastWarpError(int value) noexcept
{
#if defined(__HIP_PLATFORM_NVIDIA__)
    return __shfl_sync(0xffffffffu, value, 0);
#else
    return __shfl(value, 0);
#endif
}
