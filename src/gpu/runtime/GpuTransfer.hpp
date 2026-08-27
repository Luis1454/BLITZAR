#ifndef BLITZAR_GPU_RUNTIME_GPU_TRANSFER_HPP
#define BLITZAR_GPU_RUNTIME_GPU_TRANSFER_HPP

#include <cstddef>

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "gpu/runtime/GpuCompatibility.hpp"
#else

struct MemcpyRequest final {
    void* destination;
    const void* source;
    std::size_t bytes;
    hipMemcpyKind kind;
    hipStream_t stream;
};

inline hipError_t BlitzarMemcpyAsync(const MemcpyRequest& request) noexcept
{
    return hipMemcpyAsync(
        request.destination, request.source, request.bytes, request.kind, request.stream);
}

#endif

#endif
