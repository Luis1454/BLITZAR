#include "accelerators/gpu/hip/launch/Launch.hpp"
#include "accelerators/gpu/hip/memory/Buffers.hpp"

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "accelerators/gpu/hip/bridge/Compatibility.hpp"
#else
#include <hip/hip_runtime.h>
#endif

#include "accelerators/gpu/hip/memory/BufferCapacity.inl"
#include "accelerators/gpu/hip/memory/BufferLife.inl"
#include "accelerators/gpu/hip/memory/BufferParticleAddress.inl"
#include "accelerators/gpu/hip/memory/BufferState.inl"
#include "accelerators/gpu/hip/memory/BufferTreeAddress.inl"
#include "accelerators/gpu/hip/memory/Memory.inl"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
