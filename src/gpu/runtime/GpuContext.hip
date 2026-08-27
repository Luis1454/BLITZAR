#include "gpu/runtime/GpuContext.hpp"

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "gpu/runtime/GpuCompatibility.hpp"
#else
#include <hip/hip_runtime.h>
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <stdexcept>
#include <utility>

// clang-format off
#include "gpu/runtime/GpuTransfer.hpp"
#include "gpu/runtime/GpuRuntimeState.inl"
#include "gpu/runtime/GpuRuntimeStatus.inl"
#include "gpu/runtime/GpuRuntimeError.inl"
#include "gpu/runtime/GpuRuntimeTransfer.inl"
#include "gpu/direct/GpuDirectCompute.inl"
#include "gpu/barnes_hut/GpuBhTree.inl"
#include "gpu/barnes_hut/GpuBhCompute.inl"
#include "gpu/runtime/GpuRuntimeLife.inl"
// clang-format on
