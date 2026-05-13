/*
 * @file engine/src/cuda/MemoryPool.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public physics interfaces and data contracts for deterministic simulation kernels.
 */

#ifndef BLITZAR_ENGINE_SRC_CUDA_MEMORYPOOL_HPP_
#define BLITZAR_ENGINE_SRC_CUDA_MEMORYPOOL_HPP_
#include <cstddef>

namespace bltzr_x {
class MemoryPool {
public:
    static void initialize();
    static void destroy();
    static void* allocate(std::size_t size, void* stream = nullptr);
    static void deallocate(void* ptr, void* stream = nullptr);
    static bool isSupported();

private:
    static bool _initialized;
    static bool _supported;
    static void* _pool;
};
} // namespace bltzr_x
#endif // BLITZAR_ENGINE_SRC_CUDA_MEMORYPOOL_HPP_
