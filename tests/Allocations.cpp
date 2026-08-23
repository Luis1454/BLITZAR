#include "Check.hpp"
#include "sdk/Simulation.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>

#if defined(_WIN32)
#include <malloc.h>
#endif

namespace {

std::atomic<bool> Counting{false};
std::atomic<std::size_t> AllocationCount{0};

void* Allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t))
{
    const std::size_t actual_size = size == 0 ? 1 : size;
    void* result = nullptr;
#if defined(_WIN32)
    result = _aligned_malloc(actual_size, alignment);
#else
    if (alignment <= alignof(std::max_align_t)) {
        result = std::malloc(actual_size);
    }
    else if (posix_memalign(&result, alignment, actual_size) != 0) {
        result = nullptr;
    }
#endif
    if (result == nullptr) {
        throw std::bad_alloc();
    }
    if (Counting.load(std::memory_order_relaxed)) {
        AllocationCount.fetch_add(1, std::memory_order_relaxed);
    }
    return result;
}

void Release(void* pointer) noexcept
{
#if defined(_WIN32)
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}

} // namespace

void* operator new(std::size_t size)
{
    return Allocate(size);
}

void* operator new[](std::size_t size)
{
    return Allocate(size);
}

void* operator new(std::size_t size, std::align_val_t alignment)
{
    return Allocate(size, static_cast<std::size_t>(alignment));
}

void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return Allocate(size, static_cast<std::size_t>(alignment));
}

void operator delete(void* pointer) noexcept
{
    Release(pointer);
}

void operator delete[](void* pointer) noexcept
{
    Release(pointer);
}

void operator delete(void* pointer, std::size_t) noexcept
{
    Release(pointer);
}

void operator delete[](void* pointer, std::size_t) noexcept
{
    Release(pointer);
}

void operator delete(void* pointer, std::align_val_t) noexcept
{
    Release(pointer);
}

void operator delete[](void* pointer, std::align_val_t) noexcept
{
    Release(pointer);
}

void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept
{
    Release(pointer);
}

void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept
{
    Release(pointer);
}

int main()
{
    constexpr std::size_t ParticleCount = 2;
    std::array<double, ParticleCount> position_x{0.0, 1.0};
    std::array<double, ParticleCount> position_y{0.0, 0.0};
    std::array<double, ParticleCount> position_z{0.0, 0.0};
    std::array<double, ParticleCount> velocity_x{0.0, 0.0};
    std::array<double, ParticleCount> velocity_y{0.0, 0.0};
    std::array<double, ParticleCount> velocity_z{0.0, 0.0};
    std::array<double, ParticleCount> mass{1.0, 1.0};
    blitzar_sdk::Simulation simulation(ParticleCount);

    BLITZAR_CHECK(simulation.SetSolver(BLITZAR_SOLVER_DIRECT) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(simulation.SetGravity(1.0, 0.1) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(simulation.SetTimestep(0.01) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(simulation.SetParticles(position_x, position_y, position_z, velocity_x,
                      velocity_y, velocity_z, mass) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(simulation.Step() == BLITZAR_STATUS_OK);

    AllocationCount.store(0, std::memory_order_relaxed);
    Counting.store(true, std::memory_order_release);

    const blitzar_status first_step = simulation.Step();
    const blitzar_status second_step = simulation.Step();

    Counting.store(false, std::memory_order_release);

    BLITZAR_CHECK(first_step == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(second_step == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(AllocationCount.load(std::memory_order_relaxed) == 0);

    return 0;
}
