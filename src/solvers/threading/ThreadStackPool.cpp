#include "solvers/threading/ThreadStackPool.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace blitzar_solver_threading {

std::size_t ThreadStackPool::DetectThreadCount() noexcept
{
#if defined(_OPENMP)
    const int available_threads = omp_get_max_threads();

    return available_threads > 0 ? static_cast<std::size_t>(available_threads) : std::size_t{1};
#else
    return 1;
#endif
}

std::size_t ThreadStackPool::CalculateStackCapacity(
    std::size_t max_cells, std::size_t max_depth) noexcept
{
    if (max_cells == 0) {
        return 0;
    }

    constexpr std::size_t ChildrenPerCell = 8;
    const std::size_t maximum = std::numeric_limits<std::size_t>::max();
    const std::size_t depth_capacity = max_depth > (maximum - 1) / (ChildrenPerCell - 1)
                                           ? maximum
                                           : 1 + max_depth * (ChildrenPerCell - 1);

    return std::min(max_cells, depth_capacity);
}

ThreadStackPool::ThreadStackPool(std::size_t max_cells, std::size_t max_depth)
    : max_cells_(max_cells), max_depth_(max_depth),
      stack_capacity_(CalculateStackCapacity(max_cells, max_depth)),
      thread_count_(DetectThreadCount()), storage_{}
{
    if (stack_capacity_ != 0 &&
        thread_count_ > std::numeric_limits<std::size_t>::max() / stack_capacity_) {
        throw std::length_error("Barnes-Hut thread stack pool is too large");
    }
    storage_.resize(thread_count_ * stack_capacity_);
}

std::size_t ThreadStackPool::MaxCells() const noexcept
{
    return max_cells_;
}

std::size_t ThreadStackPool::MaxDepth() const noexcept
{
    return max_depth_;
}

std::size_t ThreadStackPool::StackCapacity() const noexcept
{
    return stack_capacity_;
}

std::size_t ThreadStackPool::ThreadCount() const noexcept
{
    return thread_count_;
}

std::span<std::size_t> ThreadStackPool::Stack(std::size_t thread_index) noexcept
{
    if (thread_index >= thread_count_ || stack_capacity_ == 0) {
        return {};
    }

    return std::span<std::size_t>(storage_).subspan(
        thread_index * stack_capacity_, stack_capacity_);
}

std::size_t ThreadStackPool::CurrentThread() noexcept
{
#if defined(_OPENMP)
    const int thread_index = omp_get_thread_num();

    return thread_index >= 0 ? static_cast<std::size_t>(thread_index) : 0;
#else
    return 0;
#endif
}

} // namespace blitzar_solver_threading
