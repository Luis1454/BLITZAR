/*
 * @file engine/physics/cuda/buffer/CudDeviceMemory.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief RAII ownership declarations for CUDA resources.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_PHYSICS_CUDA_DEVICEMEMORY_HPP_
#define BLITZAR_ENGINE_INCLUDE_PHYSICS_CUDA_DEVICEMEMORY_HPP_

#include <cstddef>

namespace blitzar_cuda_memory {

void* allocate(std::size_t size, void* stream = nullptr) noexcept;
void deallocate(void* pointer, void* stream = nullptr) noexcept;
void releaseMappedHost(void* pointer) noexcept;
void releaseGraphExec(void* handle) noexcept;
void releaseStream(void* handle) noexcept;
void releaseFftPlan(int handle) noexcept;

} // namespace blitzar_cuda_memory

#include "physics/cuda/core/CudHandles.inl"
#include "physics/cuda/buffer/CudDeviceBuffers.inl"

#endif // BLITZAR_ENGINE_INCLUDE_PHYSICS_CUDA_DEVICEMEMORY_HPP_
