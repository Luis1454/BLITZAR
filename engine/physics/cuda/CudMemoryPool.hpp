/*
 * @file engine/physics/cuda/CudMemoryPool.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public physics interfaces and data contracts for deterministic simulation kernels.
 */

#ifndef BLITZAR_ENGINE_SRC_CUDA_MEMORYPOOL_HPP_
#define BLITZAR_ENGINE_SRC_CUDA_MEMORYPOOL_HPP_

#include "CudDeviceMemory.hpp"

#include <cstddef>

namespace bltzr_x {
class MemoryPool {
public:
    static void initialize();
    static void destroy();
    static void* allocate(std::size_t size, void* stream = nullptr);
    static void deallocate(void* ptr, void* stream = nullptr);

    template <typename T>
    static void deallocate(blitzar_cuda_memory::DeviceBuffer<T>& buffer);

    static bool isSupported();

private:
    static bool _initialized;
    static bool _supported;
};
} // namespace bltzr_x

#include "CudMemoryPool.inl"

#endif // BLITZAR_ENGINE_SRC_CUDA_MEMORYPOOL_HPP_
