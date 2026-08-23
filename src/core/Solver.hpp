#ifndef BLITZAR_CORE_SOLVER_HPP
#define BLITZAR_CORE_SOLVER_HPP

#include <cstdint>
#include <cstddef>

namespace blitzar_core {

enum class SolverKind : std::uint8_t { Direct = 0, BarnesHut = 1, Fmm = 2, Pm = 3, TreePm = 4 };

struct ForceRange final {
    std::size_t source_begin{};
    std::size_t source_end{};
    bool accumulate{};

    [[nodiscard]] bool IsValid(std::size_t source_count) const noexcept
    {
        return source_begin <= source_end && source_end <= source_count;
    }
};

} // namespace blitzar_core

#endif
