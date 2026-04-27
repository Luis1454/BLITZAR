// File: engine/include/physics/CudaMemoryPool.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_CUDAMEMORYPOOL_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_CUDAMEMORYPOOL_HPP_
#include <cstddef>

namespace grav_x {
/// Description: Defines the CudaMemoryPool data or behavior contract.
class CudaMemoryPool {
public:
    /// Description: Describes the initialize operation contract.
    static void initialize();
    /// Description: Describes the destroy operation contract.
    static void destroy();
    /// Description: Describes the allocate operation contract.
    static void* allocate(std::size_t size, void* stream = nullptr);
    /// Description: Describes the deallocate operation contract.
    static void deallocate(void* ptr, void* stream = nullptr);
    /// Description: Describes the is supported operation contract.
    static bool isSupported();

private:
    static bool _initialized;
    static bool _supported;
    static void* _pool;
};
} // namespace grav_x
#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_CUDAMEMORYPOOL_HPP_
