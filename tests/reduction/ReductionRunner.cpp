#include "fixtures/FixtureCheck.hpp"
#include "reduction/ReductionBenchmark.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <vector>

namespace {

std::string_view ReductionText(blitzar_physics::ReductionKind reduction) noexcept
{
    switch (reduction) {
    case blitzar_physics::ReductionKind::Plain:

        return "plain-v1";

    case blitzar_physics::ReductionKind::Kahan:

        return "kahan-v1";

    case blitzar_physics::ReductionKind::Neumaier:

        return "neumaier-v1";
    }

    return "unknown";
}

std::string_view WorkloadText(blitzar_reduction::WorkloadKind workload) noexcept
{
    switch (workload) {
    case blitzar_reduction::WorkloadKind::Force:

        return "force";

    case blitzar_reduction::WorkloadKind::Kinetic:

        return "kinetic-energy";

    case blitzar_reduction::WorkloadKind::Potential:

        return "potential-energy";

    case blitzar_reduction::WorkloadKind::Momentum:

        return "momentum";
    }

    return "unknown";
}

void PrintReduction(const blitzar_reduction::ReductionResult& result) noexcept
{
    const std::string_view workload = WorkloadText(result.workload);
    const std::string_view reduction = ReductionText(result.reduction);

    std::fprintf(stdout,
        "BLITZAR REDUCTION schema=1 seed=%llu workload=%.*s terms=%zu policy=%.*s "
        "elapsed_ns=%llu terms_per_second=%.17g expected=%.17g value=%.17g "
        "absolute_error=%.17g relative_error=%.17g input_hash=%llu value_hash=%llu "
        "repeatable=%d finite=%d vectorization_eligible=%d selected=%d\n",
        static_cast<unsigned long long>(result.seed), static_cast<int>(workload.size()),
        workload.data(), result.term_count, static_cast<int>(reduction.size()), reduction.data(),
        static_cast<unsigned long long>(result.elapsed_ns), result.terms_per_second,
        result.expected, result.value, result.absolute_error, result.relative_error,
        static_cast<unsigned long long>(result.input_hash),
        static_cast<unsigned long long>(result.value_hash), result.repeatable ? 1 : 0,
        result.finite ? 1 : 0, result.vectorization_eligible ? 1 : 0, result.selected ? 1 : 0);
}

void PrintLongRun(const blitzar_reduction::LongRunResult& result) noexcept
{
    const std::string_view reduction = ReductionText(result.reduction);

    std::fprintf(stdout,
        "BLITZAR REDUCTION_LONG schema=1 seed=%llu steps=%zu policy=%.*s "
        "max_relative_energy_error=%.17g final_energy=%.17g final_momentum_norm=%.17g "
        "state_hash=%llu finite=%d default_policy_match=%d selected=%d\n",
        static_cast<unsigned long long>(result.seed), result.steps,
        static_cast<int>(reduction.size()), reduction.data(), result.max_relative_energy_error,
        result.final_energy, result.final_momentum_norm,
        static_cast<unsigned long long>(result.state_hash), result.finite ? 1 : 0,
        result.default_policy_match ? 1 : 0, result.selected ? 1 : 0);
}

} // namespace

int main()
{
    constexpr std::uint64_t seed = 424242;
    constexpr std::array<std::size_t, 1> term_counts{65536};
    blitzar_reduction::ReductionBenchmark benchmark(seed);
    std::vector<blitzar_reduction::ReductionResult> results;
    std::vector<blitzar_reduction::LongRunResult> long_runs;

    BLITZAR_CHECK(benchmark.Run(term_counts, results, long_runs));
    BLITZAR_CHECK(results.size() == 12);
    BLITZAR_CHECK(long_runs.size() == 3);

    for (const auto& result : results) {
        BLITZAR_CHECK(result.seed == seed);
        BLITZAR_CHECK(result.elapsed_ns > 0);
        BLITZAR_CHECK(result.finite && result.repeatable);
        BLITZAR_CHECK(std::isfinite(result.terms_per_second));
        BLITZAR_CHECK(std::isfinite(result.absolute_error) && std::isfinite(result.relative_error));
        BLITZAR_CHECK(result.selected ==
                      (result.workload == blitzar_reduction::WorkloadKind::Force
                              ? result.reduction == blitzar_physics::ReductionKind::Plain
                              : result.reduction == blitzar_physics::ReductionKind::Neumaier));

        PrintReduction(result);
    }

    for (const auto& result : long_runs) {
        BLITZAR_CHECK(result.seed == seed && result.steps == 4096);
        BLITZAR_CHECK(result.finite && result.default_policy_match);
        BLITZAR_CHECK(std::isfinite(result.max_relative_energy_error));
        BLITZAR_CHECK(
            std::isfinite(result.final_energy) && std::isfinite(result.final_momentum_norm));

        BLITZAR_CHECK(
            result.selected == (result.reduction == blitzar_physics::ReductionKind::Neumaier));

        PrintLongRun(result);
    }

    return 0;
}
