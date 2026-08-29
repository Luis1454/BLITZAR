#ifndef BLITZAR_TESTS_REDUCTION_REDUCTION_BENCHMARK_HPP
#define BLITZAR_TESTS_REDUCTION_REDUCTION_BENCHMARK_HPP

#include "physics/reduction/ScalarReduction.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace blitzar_reduction {

enum class WorkloadKind : std::uint8_t { Force, Kinetic, Potential, Momentum };

struct ReductionResult final {
    std::uint64_t seed{};
    std::size_t term_count{};
    WorkloadKind workload{};
    blitzar_physics::ReductionKind reduction{};
    std::uint64_t elapsed_ns{};
    double terms_per_second{};
    double expected{};
    double value{};
    double absolute_error{};
    double relative_error{};
    std::uint64_t input_hash{};
    std::uint64_t value_hash{};
    bool repeatable{};
    bool finite{};
    bool vectorization_eligible{};
    bool selected{};
};

struct LongRunResult final {
    std::uint64_t seed{};
    std::size_t steps{};
    blitzar_physics::ReductionKind reduction{};
    double max_relative_energy_error{};
    double final_energy{};
    double final_momentum_norm{};
    std::uint64_t state_hash{};
    bool finite{};
    bool default_policy_match{};
    bool selected{};
};

class ReductionBenchmark final {
public:
    explicit ReductionBenchmark(std::uint64_t seed);

    [[nodiscard]] bool Run(std::span<const std::size_t> term_counts,
        std::vector<ReductionResult>& results, std::vector<LongRunResult>& long_runs) const;

private:
    [[nodiscard]] bool RunCount(
        std::size_t term_count, std::vector<ReductionResult>& results) const;
    [[nodiscard]] bool Measure(WorkloadKind workload, std::span<const double> terms,
        double expected, std::vector<ReductionResult>& results) const;
    [[nodiscard]] bool RunLongRun(std::vector<LongRunResult>& results) const;

    std::uint64_t seed_{};
};

} // namespace blitzar_reduction

#endif
