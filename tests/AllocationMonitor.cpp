#include "AllocationMonitor.hpp"

#include <atomic>
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

namespace blitzar_tests {

void BeginAllocationCounting() noexcept
{
    AllocationCount.store(0, std::memory_order_relaxed);
    Counting.store(true, std::memory_order_release);
}

std::size_t EndAllocationCounting() noexcept
{
    Counting.store(false, std::memory_order_release);

    return AllocationCount.load(std::memory_order_relaxed);
}

} // namespace blitzar_tests
