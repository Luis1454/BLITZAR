#ifndef GRAVITY_ENGINE_INCLUDE_PHYSICS_CUDAMEMORYPOOL_HPP_
#define GRAVITY_ENGINE_INCLUDE_PHYSICS_CUDAMEMORYPOOL_HPP_
#include <cstddef>
namespace grav_x {
class CudaMemoryPool {
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
} // namespace grav_x
#endif // GRAVITY_ENGINE_INCLUDE_PHYSICS_CUDAMEMORYPOOL_HPP_
