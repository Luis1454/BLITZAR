#ifndef BLITZAR_TESTS_LAYOUT_LAYOUT_BENCHMARK_HPP
#define BLITZAR_TESTS_LAYOUT_LAYOUT_BENCHMARK_HPP

#include "layout/LayoutOrder.hpp"
#include "layout/LayoutStorage.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace blitzar_layout {

struct LayoutResult final {
    std::uint64_t seed{};
    std::size_t particle_count{};
    OrderKind ordering{};
    LayoutKind layout{};
    std::size_t tile_width{};
    std::uint64_t sort_ns{};
    std::uint64_t materialize_ns{};
    std::uint64_t tree_build_ns{};
    std::uint64_t scan_ns{};
    double scan_particles_per_second{};
    double locality_mean_squared_distance{};
    std::size_t cache_line_visits_proxy{};
    std::size_t candidate_bytes{};
    std::size_t materialized_bytes{};
    std::uint64_t order_hash{};
    std::uint64_t state_hash{};
    std::uint64_t byte_hash{};
    std::uint64_t tree_hash{};
    double scan_checksum{};
    bool stable{};
    bool repeatable{};
    bool ordering_equivalent{};
    bool representation_equivalent{};
    bool tree_valid{};
};

struct LayoutRun final {
    const LayoutState& state;
    const LayoutOrder& order;
    OrderKind ordering{};
    std::uint64_t sort_ns{};
    bool ordering_equivalent{};
};

struct LayoutRequest final {
    const LayoutState& state;
    const LayoutOrder& order;
    OrderKind ordering{};
    LayoutSpec specification{};
    std::uint64_t sort_ns{};
    bool ordering_equivalent{};
};

class LayoutBenchmark final {
public:
    explicit LayoutBenchmark(std::uint64_t seed);

    [[nodiscard]] bool Run(
        std::span<const std::size_t> particle_counts, std::vector<LayoutResult>& results) const;

private:
    [[nodiscard]] bool RunCount(
        std::size_t particle_count, std::vector<LayoutResult>& results) const;
    [[nodiscard]] bool RunOrder(const LayoutRun& run, std::span<const LayoutSpec> specifications,
        std::vector<LayoutResult>& results) const;
    [[nodiscard]] bool Measure(const LayoutRequest& request, LayoutResult& result) const;
    [[nodiscard]] bool ValidateCount(std::span<LayoutResult> results) const noexcept;

    std::uint64_t seed_{};
};

} // namespace blitzar_layout

#endif
