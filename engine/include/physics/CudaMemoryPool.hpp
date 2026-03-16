#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_CUDAMEMORYPOOL_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_CUDAMEMORYPOOL_HPP_

#include <cuda_runtime.h>
#include <cstddef>

namespace grav_x {

class CudaMemoryPool {
public:
    static void initialize();
    static void destroy();

    static void* allocate(std::size_t size, cudaStream_t stream = 0);
    static void deallocate(void* ptr, cudaStream_t stream = 0);

    static bool isSupported();

private:
    static bool _initialized;
    static bool _supported;
    static cudaMemPool_t _pool;
};

} // namespace grav_x

#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_CUDAMEMORYPOOL_HPP_
