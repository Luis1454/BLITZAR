#include "gpu/runtime/GpuLaunch.hpp"

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "gpu/runtime/GpuCompatibility.hpp"
#else
#include <hip/hip_runtime.h>
#endif

// clang-format off
#include "gpu/runtime/GpuCudError.inl"
#include "gpu/barnes_hut/GpuBhDevice.inl"
#include "gpu/barnes_hut/GpuBhLaunch.inl"
// clang-format on
