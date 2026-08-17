/*
 * @file engine/physics/cuda/buffer/CudMemoryPool.cu
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

#include "physics/cuda/buffer/CudMemoryPool.hpp"
#include <cstdint>
#include <cstdio>
#include <cuda_runtime.h>

namespace bltzr_x {
bool MemoryPool::_initialized = false;
bool MemoryPool::_supported = false;

void MemoryPool::initialize()
{
    if (_initialized)
        return;
    int device = 0;
    if (cudaGetDevice(&device) != cudaSuccess && cudaSetDevice(0) != cudaSuccess) {
        _initialized = true;
        _supported = false;
        return;
    }
    if (cudaGetDevice(&device) != cudaSuccess) {
        _initialized = true;
        _supported = false;
        return;
    }
    int supportsAsync = 0;
    if (cudaDeviceGetAttribute(&supportsAsync, cudaDevAttrMemoryPoolsSupported, device) !=
        cudaSuccess) {
        _initialized = true;
        _supported = false;
        return;
    }
    if (supportsAsync) {
        cudaMemPool_t pool = nullptr;
        cudaError_t err = cudaDeviceGetDefaultMemPool(&pool, device);
        if (err == cudaSuccess && pool != nullptr) {
            _supported = true;
            // Set release threshold to prevent excessive memory retention
            uint64_t threshold = 256 * 1024 * 1024;
            cudaMemPoolSetAttribute(pool, cudaMemPoolAttrReleaseThreshold, &threshold);
        }
        else {
            _supported = false;
        }
    }
    else {
        _supported = false;
    }
    _initialized = true;
}

void MemoryPool::destroy()
{
    _initialized = false;
    _supported = false;
}

void* MemoryPool::allocate(std::size_t size, void* stream)
{
    if (!_initialized)
        initialize();
    void* ptr = nullptr;
    cudaError_t err;
    if (_supported)
        err = cudaMallocAsync(&ptr, size, static_cast<cudaStream_t>(stream));
    else {
        err = cudaMalloc(&ptr, size);
    }
    if (err != cudaSuccess)
        return nullptr;
    // fprintf(stdout, "[cuda-pool] allocated %zu bytes at %p\n", size, ptr);
    return ptr;
}

void MemoryPool::deallocate(void* ptr, void* stream)
{
    if (!ptr)
        return;
    if (_supported) {
        cudaFreeAsync(ptr, static_cast<cudaStream_t>(stream));
    }
    else {
        cudaFree(ptr);
    }
}

bool MemoryPool::isSupported()
{
    if (!_initialized)
        initialize();
    return _supported;
}
} // namespace bltzr_x
