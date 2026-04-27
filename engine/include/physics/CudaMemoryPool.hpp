// File: engine/include/physics/CudaMemoryPool.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_CUDAMEMORYPOOL_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_CUDAMEMORYPOOL_HPP_
#include <cstddef>
namespace grav_x {
/// Description: Defines the CudaMemoryPool data or behavior contract.
class CudaMemoryPool {
public:
    /// Description: Executes the initialize operation.
    static void initialize();
    /// Description: Executes the destroy operation.
    static void destroy();
    /// Description: Executes the allocate operation.
    static void* allocate(std::size_t size, void* stream = nullptr);
    /// Description: Executes the deallocate operation.
    static void deallocate(void* ptr, void* stream = nullptr);
    /// Description: Executes the isSupported operation.
    static bool isSupported();

private:
    static bool _initialized;
    static bool _supported;
    static void* _pool;
};
} // namespace grav_x
#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_CUDAMEMORYPOOL_HPP_
