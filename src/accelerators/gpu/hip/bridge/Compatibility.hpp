#ifndef BLITZAR_ACCELERATORS_GPU_HIP_BRIDGE_COMPATIBILITY_HPP
#define BLITZAR_ACCELERATORS_GPU_HIP_BRIDGE_COMPATIBILITY_HPP

#include <cstddef>
#include <cuda_runtime.h>

using hipError_t = cudaError_t;
using hipStream_t = cudaStream_t;

constexpr hipError_t hipSuccess = cudaSuccess;
constexpr hipError_t hipErrorMemoryAllocation = cudaErrorMemoryAllocation;
constexpr unsigned int hipHostMallocDefault = cudaHostAllocDefault;
constexpr cudaMemcpyKind hipMemcpyHostToDevice = cudaMemcpyHostToDevice;
constexpr cudaMemcpyKind hipMemcpyDeviceToHost = cudaMemcpyDeviceToHost;
constexpr unsigned int hipStreamNonBlocking = cudaStreamNonBlocking;

inline hipError_t hipHostMalloc(void** pointer, std::size_t bytes, unsigned int flags) noexcept
{
    return cudaHostAlloc(pointer, bytes, flags);
}

inline hipError_t hipHostFree(void* pointer) noexcept
{
    return cudaFreeHost(pointer);
}

inline hipError_t hipMalloc(void** pointer, std::size_t bytes) noexcept
{
    return cudaMalloc(pointer, bytes);
}

inline hipError_t hipFree(void* pointer) noexcept
{
    return cudaFree(pointer);
}

inline hipError_t hipGetDeviceCount(int* count) noexcept
{
    return cudaGetDeviceCount(count);
}

inline hipError_t hipSetDevice(int device) noexcept
{
    return cudaSetDevice(device);
}

inline hipError_t hipStreamCreateWithFlags(hipStream_t* stream, unsigned int flags) noexcept
{
    return cudaStreamCreateWithFlags(stream, flags);
}

inline hipError_t hipStreamSynchronize(hipStream_t stream) noexcept
{
    return cudaStreamSynchronize(stream);
}

inline hipError_t hipStreamDestroy(hipStream_t stream) noexcept
{
    return cudaStreamDestroy(stream);
}

struct MemcpyRequest final {
    void* destination;
    const void* source;
    std::size_t bytes;
    cudaMemcpyKind kind;
    hipStream_t stream;
};

inline hipError_t BlitzarMemcpyAsync(const MemcpyRequest& request) noexcept
{
    return cudaMemcpyAsync(
        request.destination, request.source, request.bytes, request.kind, request.stream);
}

inline hipError_t hipMemsetAsync(
    void* destination, int value, std::size_t bytes, hipStream_t stream) noexcept
{
    return cudaMemsetAsync(destination, value, bytes, stream);
}

inline hipError_t hipGetLastError() noexcept
{
    return cudaGetLastError();
}

#define hipLaunchKernelGGL(kernel, grid, block, shared, stream, ...) \
    kernel<<<(grid), (block), (shared), (stream)>>>(__VA_ARGS__)

#endif
