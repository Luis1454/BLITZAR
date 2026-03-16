#include "physics/CudaMemoryPool.hpp"
#include <cstdio>

namespace grav_x {

bool CudaMemoryPool::_initialized = false;
bool CudaMemoryPool::_supported = false;
cudaMemPool_t CudaMemoryPool::_pool = nullptr;

void CudaMemoryPool::initialize()
{
    if (_initialized) return;

    int device = 0;
    cudaGetDevice(&device);

    int supportsAsync = 0;
    cudaDeviceGetAttribute(&supportsAsync, cudaDevAttrMemoryPoolsSupported, device);

    if (supportsAsync) {
        cudaError_t err = cudaDeviceGetDefaultMemPool(&_pool, device);
        if (err == cudaSuccess) {
            _supported = true;
            // Set release threshold to prevent excessive memory retention
            // 256MB threshold for demonstration, can be tuned
            uint64_t threshold = 256 * 1024 * 1024;
            cudaMemPoolSetAttribute(_pool, cudaMemPoolAttrReleaseThreshold, &threshold);
        } else {
            fprintf(stderr, "[cuda-pool] failed to get default mempool: %s\n", cudaGetErrorString(err));
        }
    } else {
        fprintf(stdout, "[cuda-pool] async memory pools not supported on this device\n");
    }

    _initialized = true;
}

void CudaMemoryPool::destroy()
{
    // Default pool doesn't need explicit destruction, but we reset state
    _initialized = false;
    _supported = false;
    _pool = nullptr;
}

void* CudaMemoryPool::allocate(std::size_t size, cudaStream_t stream)
{
    if (!_initialized) initialize();

    void* ptr = nullptr;
    cudaError_t err;

    if (_supported) {
        err = cudaMallocAsync(&ptr, size, stream);
    } else {
        err = cudaMalloc(&ptr, size);
    }

    if (err != cudaSuccess) {
        fprintf(stderr, "[cuda-pool] allocation of %zu bytes failed: %s\n", size, cudaGetErrorString(err));
        return nullptr;
    }

    return ptr;
}

void CudaMemoryPool::deallocate(void* ptr, cudaStream_t stream)
{
    if (!ptr) return;

    if (_supported) {
        cudaFreeAsync(ptr, stream);
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
