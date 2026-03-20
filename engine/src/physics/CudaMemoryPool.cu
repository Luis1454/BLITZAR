#include "physics/CudaMemoryPool.hpp"
#include <cuda_runtime.h>
#include <cstdio>
#include <cstdint>

namespace grav_x {

bool CudaMemoryPool::_initialized = false;
bool CudaMemoryPool::_supported = false;
void* CudaMemoryPool::_pool = nullptr;

void CudaMemoryPool::initialize()
{
    if (_initialized) return;

    int device = 0;
    if (cudaGetDevice(&device) != cudaSuccess) {
        cudaSetDevice(0);
        device = 0;
    }

    int supportsAsync = 0;
    cudaDeviceGetAttribute(&supportsAsync, cudaDevAttrMemoryPoolsSupported, device);

    if (supportsAsync) {
        cudaMemPool_t pool;
        cudaError_t err = cudaDeviceGetDefaultMemPool(&pool, device);
        if (err == cudaSuccess) {
            _pool = static_cast<void*>(pool);
            _supported = true;
            // Set release threshold to prevent excessive memory retention
            uint64_t threshold = 256 * 1024 * 1024;
            cudaMemPoolSetAttribute(pool, cudaMemPoolAttrReleaseThreshold, &threshold);
        }
    }

    _initialized = true;
}

void CudaMemoryPool::destroy()
{
    _initialized = false;
    _supported = false;
    _pool = nullptr;
}

void* CudaMemoryPool::allocate(std::size_t size, void* stream)
{
    if (!_initialized) initialize();

    void* ptr = nullptr;
    cudaError_t err;

    if (_supported) {
        err = cudaMallocAsync(&ptr, size, static_cast<cudaStream_t>(stream));
    } else {
        err = cudaMalloc(&ptr, size);
    }

    if (err != cudaSuccess) {
        return nullptr;
    }

    // fprintf(stdout, "[cuda-pool] allocated %zu bytes at %p\n", size, ptr);
    return ptr;
}

void CudaMemoryPool::deallocate(void* ptr, void* stream)
{
    if (!ptr) return;

    if (_supported) {
        cudaFreeAsync(ptr, static_cast<cudaStream_t>(stream));
    } else {
        cudaFree(ptr);
    }
}

bool CudaMemoryPool::isSupported()
{
    if (!_initialized) initialize();
    return _supported;
}

} // namespace grav_x
