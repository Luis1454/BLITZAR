#include "accelerators/gpu/hip/runtime/Context.hpp"

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "accelerators/gpu/hip/bridge/Compatibility.hpp"
#else
#include <hip/hip_runtime.h>
#endif

#include "accelerators/gpu/hip/barnes_hut/BarnesContext.inl"
#include "accelerators/gpu/hip/barnes_hut/TreeContext.inl"
#include "accelerators/gpu/hip/bridge/Transfer.hpp"
#include "accelerators/gpu/hip/direct/DirectContext.inl"
#include "accelerators/gpu/hip/runtime/RuntimeError.inl"
#include "accelerators/gpu/hip/runtime/RuntimeLife.inl"
#include "accelerators/gpu/hip/runtime/RuntimeState.inl"
#include "accelerators/gpu/hip/runtime/RuntimeStatus.inl"
#include "accelerators/gpu/hip/runtime/RuntimeTransfer.inl"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <stdexcept>
#include <utility>
