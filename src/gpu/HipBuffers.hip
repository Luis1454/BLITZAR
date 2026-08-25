#include "gpu/HipBuffers.hpp"
#include "solvers/gpu/HipKernel.hpp"

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "gpu/HipCompat.hpp"
#else
#include <hip/hip_runtime.h>
#endif

#include "gpu/HipBufferCapacity.inl"
#include "gpu/HipBufferLife.inl"
#include "gpu/HipBufferParticleAddress.inl"
#include "gpu/HipBufferState.inl"
#include "gpu/HipBufferTreeAddress.inl"
#include "gpu/HipMemory.inl"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
