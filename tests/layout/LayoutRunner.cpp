#include "fixtures/FixtureCheck.hpp"
#include "layout/LayoutBenchmark.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <vector>

namespace {

std::string_view OrderText(blitzar_layout::OrderKind kind) noexcept
{
    return kind == blitzar_layout::OrderKind::StableRadix ? "stable-radix-v1"
                                                          : "stable-comparison-v1";
}

std::string_view LayoutText(blitzar_layout::LayoutKind kind) noexcept
{
    return kind == blitzar_layout::LayoutKind::Aosoa ? "aosoa" : "soa";
}

void PrintResult(const blitzar_layout::LayoutResult& result) noexcept
{
    const std::string_view ordering = OrderText(result.ordering);
    const std::string_view layout = LayoutText(result.layout);

    std::fprintf(stdout,
        "BLITZAR LAYOUT schema=1 seed=%llu particles=%zu ordering=%.*s layout=%.*s "
        "tile_width=%zu sort_ns=%llu materialize_ns=%llu tree_build_ns=%llu scan_ns=%llu "
        "scan_particles_per_second=%.17g locality_mean_squared_distance=%.17g "
        "cache_line_visits_proxy=%zu candidate_bytes=%zu materialized_bytes=%zu "
        "order_hash=%llu state_hash=%llu byte_hash=%llu tree_hash=%llu scan_checksum=%.17g "
        "stable=%d repeatable=%d ordering_equivalent=%d representation_equivalent=%d "
        "tree_valid=%d\n",
        static_cast<unsigned long long>(result.seed), result.particle_count,
        static_cast<int>(ordering.size()), ordering.data(), static_cast<int>(layout.size()),
        layout.data(), result.tile_width, static_cast<unsigned long long>(result.sort_ns),
        static_cast<unsigned long long>(result.materialize_ns),
        static_cast<unsigned long long>(result.tree_build_ns),
        static_cast<unsigned long long>(result.scan_ns), result.scan_particles_per_second,
        result.locality_mean_squared_distance, result.cache_line_visits_proxy,
        result.candidate_bytes, result.materialized_bytes,
        static_cast<unsigned long long>(result.order_hash),
        static_cast<unsigned long long>(result.state_hash),
        static_cast<unsigned long long>(result.byte_hash),
        static_cast<unsigned long long>(result.tree_hash), result.scan_checksum,
        result.stable ? 1 : 0, result.repeatable ? 1 : 0, result.ordering_equivalent ? 1 : 0,
        result.representation_equivalent ? 1 : 0, result.tree_valid ? 1 : 0);
}

} // namespace

int main()
{
    constexpr std::uint64_t seed = 424242;
    constexpr std::array<std::size_t, 3> particle_counts{65, 513, 4097};
    blitzar_layout::LayoutBenchmark benchmark(seed);
    std::vector<blitzar_layout::LayoutResult> results;

    BLITZAR_CHECK(benchmark.Run(particle_counts, results));
    BLITZAR_CHECK(results.size() == particle_counts.size() * 10U);

    for (const auto& result : results) {
        BLITZAR_CHECK(result.seed == seed);
        BLITZAR_CHECK(result.sort_ns > 0 && result.materialize_ns > 0);
        BLITZAR_CHECK(result.tree_build_ns > 0 && result.scan_ns > 0);
        BLITZAR_CHECK(result.candidate_bytes > 0 && result.cache_line_visits_proxy > 0);
        BLITZAR_CHECK(std::isfinite(result.scan_particles_per_second));
        BLITZAR_CHECK(std::isfinite(result.locality_mean_squared_distance));
        BLITZAR_CHECK(std::isfinite(result.scan_checksum));
        BLITZAR_CHECK(result.stable && result.repeatable && result.ordering_equivalent);
        BLITZAR_CHECK(result.representation_equivalent && result.tree_valid);

        PrintResult(result);
    }

    return 0;
}
