/*
 * @file engine/physics/cuda/src/DeviceMemory.cu
 * @author Luis1454
 * @project BLITZAR
 * @brief CUDA implementation for RAII allocation hooks.
 */

#include "CudMemoryPool.hpp"
#include "DeviceMemory.hpp"

#include <cufft.h>
#include <cuda_runtime.h>

namespace blitzar_cuda_memory {

void* allocate(std::size_t size, void* stream) noexcept
{
    return bltzr_x::MemoryPool::allocate(size, stream);
}

void deallocate(void* pointer, void* stream) noexcept
{
    bltzr_x::MemoryPool::deallocate(pointer, stream);
}

void releaseMappedHost(void* pointer) noexcept
{
    if (pointer != nullptr) {
        cudaFreeHost(pointer);
    }
}

void releaseGraphExec(void* handle) noexcept
{
    if (handle != nullptr) {
        cudaGraphExecDestroy(static_cast<cudaGraphExec_t>(handle));
    }
}

void releaseStream(void* handle) noexcept
{
    if (handle != nullptr) {
        cudaStreamDestroy(static_cast<cudaStream_t>(handle));
    }
}

void releaseFftPlan(int handle) noexcept
{
    if (handle != 0) {
        cufftDestroy(static_cast<cufftHandle>(handle));
    }
}

} // namespace blitzar_cuda_memory
