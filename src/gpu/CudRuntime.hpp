#ifndef BLITZAR_GPU_CUD_RUNTIME_HPP
#define BLITZAR_GPU_CUD_RUNTIME_HPP

#include <cstddef>

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "gpu/HipCompat.hpp"
#else

struct HipMemcpyRequest final {
    void* destination;
    const void* source;
    std::size_t bytes;
    hipMemcpyKind kind;
    hipStream_t stream;
};

inline hipError_t BlitzarHipMemcpyAsync(const HipMemcpyRequest& request) noexcept
{
    return hipMemcpyAsync(
        request.destination, request.source, request.bytes, request.kind, request.stream);
}

#endif

#endif
