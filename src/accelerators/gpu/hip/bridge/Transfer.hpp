#ifndef BLITZAR_ACCELERATORS_GPU_HIP_BRIDGE_TRANSFER_HPP
#define BLITZAR_ACCELERATORS_GPU_HIP_BRIDGE_TRANSFER_HPP

#include <cstddef>

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "accelerators/gpu/hip/bridge/Compatibility.hpp"
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
