#ifndef BLITZAR_CORE_CORE_EXECUTION_HPP
#define BLITZAR_CORE_CORE_EXECUTION_HPP

#include <cstdint>
#include <string_view>

namespace blitzar_core {

enum class ExecutionMode : std::uint8_t {
    Strict = 0,
    Fast = 1,
    Deterministic = Strict,
};

enum class FmaPolicy : std::uint8_t {
    Disabled = 0,
    Hardware = 1,
};

enum class ReductionPolicy : std::uint8_t {
    Ordered = 0,
    Compensated = 1,
    BackendDefined = 2,
};

struct BackendExecutionPolicy final {
    FmaPolicy fma{FmaPolicy::Disabled};
    ReductionPolicy reduction{ReductionPolicy::Ordered};

    [[nodiscard]] bool IsValid() const noexcept
    {
        const bool valid_fma = fma == FmaPolicy::Disabled || fma == FmaPolicy::Hardware;
        const bool valid_reduction = reduction == ReductionPolicy::Ordered ||
                                     reduction == ReductionPolicy::Compensated ||
                                     reduction == ReductionPolicy::BackendDefined;

        return valid_fma && valid_reduction;
    }

    [[nodiscard]] bool IsBitwiseReproducible() const noexcept
    {
        return fma == FmaPolicy::Disabled && reduction == ReductionPolicy::Ordered;
    }
};

struct ExecutionSettings final {
    std::uint64_t seed{};
    ExecutionMode mode{ExecutionMode::Strict};
    BackendExecutionPolicy cpu{};
    BackendExecutionPolicy hip{};
    BackendExecutionPolicy mpi{};

    [[nodiscard]] static constexpr ExecutionSettings Strict(std::uint64_t seed) noexcept
    {
        return {seed, ExecutionMode::Strict, {}, {}, {}};
    }

    [[nodiscard]] static constexpr ExecutionSettings Fast(std::uint64_t seed) noexcept
    {
        constexpr BackendExecutionPolicy policy{
            FmaPolicy::Hardware, ReductionPolicy::BackendDefined};

        return {seed, ExecutionMode::Fast, policy, policy, policy};
    }

    [[nodiscard]] bool IsValid() const noexcept
    {
        if ((mode != ExecutionMode::Strict && mode != ExecutionMode::Fast) || !cpu.IsValid() ||
            !hip.IsValid() || !mpi.IsValid()) {
            return false;
        }

        return mode != ExecutionMode::Strict || IsBitwiseReproducible();
    }

    [[nodiscard]] bool IsBitwiseReproducible() const noexcept
    {
        return mode == ExecutionMode::Strict && cpu.IsBitwiseReproducible() &&
               hip.IsBitwiseReproducible() && mpi.IsBitwiseReproducible();
    }
};

[[nodiscard]] inline std::string_view ExecutionModeName(ExecutionMode mode) noexcept
{
    switch (mode) {
    case ExecutionMode::Strict:

        return "strict";

    case ExecutionMode::Fast:

        return "fast";

    default:

        return {};
    }
}

[[nodiscard]] inline bool ParseExecutionMode(std::string_view text, ExecutionMode& mode) noexcept
{
    if (text == "strict") {
        mode = ExecutionMode::Strict;

        return true;
    }

    if (text == "fast") {
        mode = ExecutionMode::Fast;

        return true;
    }

    return false;
}

[[nodiscard]] inline std::string_view FmaPolicyName(FmaPolicy policy) noexcept
{
    switch (policy) {
    case FmaPolicy::Disabled:

        return "disabled";

    case FmaPolicy::Hardware:

        return "hardware";

    default:

        return {};
    }
}

[[nodiscard]] inline bool ParseFmaPolicy(std::string_view text, FmaPolicy& policy) noexcept
{
    if (text == "disabled") {
        policy = FmaPolicy::Disabled;

        return true;
    }

    if (text == "hardware") {
        policy = FmaPolicy::Hardware;

        return true;
    }

    return false;
}

[[nodiscard]] inline std::string_view ReductionPolicyName(ReductionPolicy policy) noexcept
{
    switch (policy) {
    case ReductionPolicy::Ordered:

        return "ordered";

    case ReductionPolicy::Compensated:

        return "compensated";

    case ReductionPolicy::BackendDefined:

        return "backend";

    default:

        return {};
    }
}

[[nodiscard]] inline bool ParseReductionPolicy(
    std::string_view text, ReductionPolicy& policy) noexcept
{
    if (text == "ordered") {
        policy = ReductionPolicy::Ordered;

        return true;
    }

    if (text == "compensated") {
        policy = ReductionPolicy::Compensated;

        return true;
    }

    if (text == "backend") {
        policy = ReductionPolicy::BackendDefined;

        return true;
    }

    return false;
}

} // namespace blitzar_core

#endif
