#ifndef BLITZAR_CORE_CONTRACTS_EXECUTION_HPP
#define BLITZAR_CORE_CONTRACTS_EXECUTION_HPP

#include <cstdint>

namespace blitzar_core {

enum class ExecutionMode : std::uint8_t { Deterministic = 0 };

struct ExecutionSettings final {
    std::uint64_t seed{};
    ExecutionMode mode{ExecutionMode::Deterministic};

    [[nodiscard]] bool IsValid() const noexcept
    {
        return mode == ExecutionMode::Deterministic;
    }
};

} // namespace blitzar_core

#endif
