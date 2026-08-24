#ifndef BLITZAR_SOLVERS_BARNES_HUT_THREAD_STACK_POOL_HPP
#define BLITZAR_SOLVERS_BARNES_HUT_THREAD_STACK_POOL_HPP

#include <cstddef>
#include <span>
#include <vector>

namespace blitzar_barnes_hut {

class ThreadStackPool final {
public:
    ThreadStackPool(std::size_t max_cells, std::size_t max_depth);

    ThreadStackPool(const ThreadStackPool&) = delete;
    ThreadStackPool& operator=(const ThreadStackPool&) = delete;
    ThreadStackPool(ThreadStackPool&&) noexcept = default;
    ThreadStackPool& operator=(ThreadStackPool&&) noexcept = default;
    ~ThreadStackPool() = default;

    [[nodiscard]] std::size_t MaxCells() const noexcept;
    [[nodiscard]] std::size_t MaxDepth() const noexcept;
    [[nodiscard]] std::size_t StackCapacity() const noexcept;
    [[nodiscard]] std::size_t ThreadCount() const noexcept;
    [[nodiscard]] std::span<std::size_t> Stack(std::size_t thread_index) noexcept;

    [[nodiscard]] static std::size_t CurrentThread() noexcept;

private:
    [[nodiscard]] static std::size_t DetectThreadCount() noexcept;
    [[nodiscard]] static std::size_t CalculateStackCapacity(
        std::size_t max_cells, std::size_t max_depth) noexcept;

    std::size_t max_cells_;
    std::size_t max_depth_;
    std::size_t stack_capacity_;
    std::size_t thread_count_;
    std::vector<std::size_t> storage_;
};

} // namespace blitzar_barnes_hut

#endif
