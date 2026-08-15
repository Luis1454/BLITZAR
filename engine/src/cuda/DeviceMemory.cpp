/*
 * @file engine/src/cuda/DeviceMemory.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief CPU fallback for CUDA ownership hooks.
 */

#include "physics/cuda/DeviceMemory.hpp"

namespace blitzar_cuda_memory {

void* allocate(std::size_t, void*) noexcept
{
    return nullptr;
}

void deallocate(void*, void*) noexcept
{
}

void releaseMappedHost(void*) noexcept
{
}

void releaseGraphExec(void*) noexcept
{
}

void releaseStream(void*) noexcept
{
}

void releaseFftPlan(int) noexcept
{
}

} // namespace blitzar_cuda_memory
