#include "gpu/HipContext.hpp"

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "gpu/HipCompat.hpp"
#else
#include <hip/hip_runtime.h>
#endif

#include "gpu/CudRuntime.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <stdexcept>
#include <utility>

#include "gpu/HipContextStatus.inl"
#include "gpu/HipContextState.inl"
#include "gpu/HipContextTransfer.inl"
#include "gpu/HipContextError.inl"
#include "gpu/HipContextTree.inl"
#include "gpu/HipContextDirect.inl"
#include "gpu/HipContextBarnes.inl"
#include "gpu/HipContextLife.inl"
