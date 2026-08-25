#include "solvers/gpu/HipKernel.hpp"

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "gpu/HipCompat.hpp"
#else
#include <hip/hip_runtime.h>
#endif

#include "solvers/gpu/BarnesHutDevice.inl"
#include "solvers/gpu/BarnesHutLaunch.inl"
#include "solvers/gpu/CudError.inl"
