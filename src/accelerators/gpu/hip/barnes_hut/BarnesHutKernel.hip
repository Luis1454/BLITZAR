#include "accelerators/gpu/hip/launch/Launch.hpp"

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "accelerators/gpu/hip/bridge/Compatibility.hpp"
#else
#include <hip/hip_runtime.h>
#endif

#include "accelerators/gpu/hip/barnes_hut/BarnesHutDevice.inl"
#include "accelerators/gpu/hip/barnes_hut/BarnesHutLaunch.inl"
#include "accelerators/gpu/hip/launch/CudError.inl"
