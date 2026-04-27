// File: engine/src/physics/CudaMemoryPool.cu
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "physics/CudaMemoryPool.hpp"
#include <cstdint>
#include <cstdio>
#include <cuda_runtime.h>

namespace grav_x {
bool CudaMemoryPool::_initialized = false;
bool CudaMemoryPool::_supported = false;
void* CudaMemoryPool::_pool = nullptr;

/// Description: Executes the initialize operation.
void CudaMemoryPool::initialize()
{
    if (_initialized)
        return;
    int device = 0;
    if (cudaGetDevice(&device) != cudaSuccess && cudaSetDevice(0) != cudaSuccess) {
        _initialized = true;
        _supported = false;
        _pool = nullptr;
        return;
    }
    if (cudaGetDevice(&device) != cudaSuccess) {
        _initialized = true;
        _supported = false;
        _pool = nullptr;
        return;
    }
    int supportsAsync = 0;
    if (cudaDeviceGetAttribute(&supportsAsync, cudaDevAttrMemoryPoolsSupported, device) !=
        cudaSuccess) {
        _initialized = true;
        _supported = false;
        _pool = nullptr;
        return;
    }
    if (supportsAsync) {
        cudaMemPool_t pool = nullptr;
        cudaError_t err = cudaDeviceGetDefaultMemPool(&pool, device);
        if (err == cudaSuccess && pool != nullptr) {
            _pool = static_cast<void*>(pool);
            _supported = true;
            // Set release threshold to prevent excessive memory retention
            uint64_t threshold = 256 * 1024 * 1024;
            cudaMemPoolSetAttribute(pool, cudaMemPoolAttrReleaseThreshold, &threshold);
        }
        else {
            _supported = false;
            _pool = nullptr;
        }
    }
    else {
        _supported = false;
        _pool = nullptr;
    }
    _initialized = true;
}

/// Description: Executes the destroy operation.
void CudaMemoryPool::destroy()
{
    _initialized = false;
    _supported = false;
    _pool = nullptr;
}

/// Description: Executes the allocate operation.
void* CudaMemoryPool::allocate(std::size_t size, void* stream)
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

/// Description: Executes the deallocate operation.
void CudaMemoryPool::deallocate(void* ptr, void* stream)
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

/// Description: Executes the isSupported operation.
bool CudaMemoryPool::isSupported()
{
    if (!_initialized)
        initialize();
    return _supported;
}
} // namespace grav_x
