#include "gpu/runtime/GpuLaunch.hpp"

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "gpu/runtime/GpuCompatibility.hpp"
#else
#include <hip/hip_runtime.h>
#endif

#include "gpu/direct/GpuDirectDevice.inl"
#include "gpu/direct/GpuDirectLaunch.inl"
#include "gpu/runtime/GpuCudError.inl"
