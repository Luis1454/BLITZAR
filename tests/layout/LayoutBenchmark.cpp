#include "layout/LayoutBenchmark.hpp"

#include "trees/octree/Octree.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <limits>

namespace blitzar_layout {

namespace {

std::uint64_t Elapsed(
    std::chrono::steady_clock::time_point begin, std::chrono::steady_clock::time_point end) noexcept
{
    const auto value = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();

    return value > 0 ? static_cast<std::uint64_t>(value) : 1U;
}

std::size_t MaxCells(std::size_t particle_count) noexcept
{
    if (particle_count > (std::numeric_limits<std::size_t>::max() - 1U) / 8U) {
        return 0;
    }

    return particle_count * 8U + 1U;
}

std::uint64_t TreeHash(const blitzar_trees::Octree& tree) noexcept
{
    std::uint64_t hash = 1469598103934665603ULL;
    const auto append_scalar = [&hash](const blitzar_core::Scalar value) noexcept {
        hash ^= std::bit_cast<std::uint64_t>(value);
        hash *= 1099511628211ULL;
    };

    const auto append_size = [&hash](const std::size_t value) noexcept {
        hash ^= static_cast<std::uint64_t>(value);
        hash *= 1099511628211ULL;
    };

    for (const auto& cell : tree.Cells()) {
        append_scalar(cell.center.x);
        append_scalar(cell.center.y);
        append_scalar(cell.center.z);
        append_scalar(cell.center_of_mass.x);
        append_scalar(cell.center_of_mass.y);
        append_scalar(cell.center_of_mass.z);
        append_scalar(cell.half_extent);
        append_scalar(cell.mass);
        append_size(cell.begin);
        append_size(cell.count);
        append_size(cell.depth);

        for (const std::size_t child : cell.children) {
            append_size(child);
        }
    }

    for (const std::size_t index : tree.Indices()) {
        append_size(index);
    }

    return hash;
}

std::array<LayoutSpec, 5> Specifications() noexcept
{
    return {{{LayoutKind::Soa, 0}, {LayoutKind::Aosoa, 4}, {LayoutKind::Aosoa, 8},
        {LayoutKind::Aosoa, 16}, {LayoutKind::Aosoa, 32}}};
}

} // namespace

LayoutBenchmark::LayoutBenchmark(std::uint64_t seed) : seed_(seed) {}

bool LayoutBenchmark::Run(
    std::span<const std::size_t> particle_counts, std::vector<LayoutResult>& results) const
{
    if (particle_counts.empty() ||
        particle_counts.size() > std::numeric_limits<std::size_t>::max() / 10U) {
        return false;
    }

    results.clear();
    results.reserve(particle_counts.size() * 10U);

    for (const std::size_t particle_count : particle_counts) {
        if (!RunCount(particle_count, results)) {
            return false;
        }
    }

    return results.size() == particle_counts.size() * 10U;
}

bool LayoutBenchmark::RunCount(std::size_t particle_count, std::vector<LayoutResult>& results) const
{
    if (particle_count == 0 || MaxCells(particle_count) == 0) {
        return false;
    }

    LayoutState state(particle_count, seed_);
    LayoutOrder comparison(particle_count);
    LayoutOrder radix(particle_count);
    LayoutOrder comparison_repeat(particle_count);
    LayoutOrder radix_repeat(particle_count);
    const auto comparison_begin = std::chrono::steady_clock::now();

    comparison.Build(state, OrderKind::StableComparison);

    const auto comparison_end = std::chrono::steady_clock::now();
    const auto radix_begin = std::chrono::steady_clock::now();

    radix.Build(state, OrderKind::StableRadix);

    const auto radix_end = std::chrono::steady_clock::now();

    comparison_repeat.Build(state, OrderKind::StableComparison);
    radix_repeat.Build(state, OrderKind::StableRadix);

    if (!comparison.IsStable() || !radix.IsStable() || !comparison.Matches(radix) ||
        !comparison.Matches(comparison_repeat) || !radix.Matches(radix_repeat)) {
        return false;
    }

    const std::size_t begin = results.size();
    const auto specifications = Specifications();
    const LayoutRun comparison_run{state, comparison, OrderKind::StableComparison,
        Elapsed(comparison_begin, comparison_end), true};

    const LayoutRun radix_run{
        state, radix, OrderKind::StableRadix, Elapsed(radix_begin, radix_end), true};

    if (!RunOrder(comparison_run, specifications, results) ||
        !RunOrder(radix_run, specifications, results)) {
        return false;
    }

    return ValidateCount(std::span<LayoutResult>(results).subspan(begin, results.size() - begin));
}

bool LayoutBenchmark::RunOrder(const LayoutRun& run, std::span<const LayoutSpec> specifications,
    std::vector<LayoutResult>& results) const
{
    for (const LayoutSpec specification : specifications) {
        LayoutResult result{};
        const LayoutRequest request{run.state, run.order, run.ordering, specification, run.sort_ns,
            run.ordering_equivalent};

        if (!Measure(request, result)) {
            return false;
        }

        results.push_back(result);
    }

    return true;
}

bool LayoutBenchmark::Measure(const LayoutRequest& request, LayoutResult& result) const
{
    LayoutStorage storage(
        request.state.Count(), request.specification.kind, request.specification.tile_width);

    if (!storage.IsValid()) {
        return false;
    }

    const auto materialize_begin = std::chrono::steady_clock::now();

    if (!storage.Load(request.state, request.order.Values())) {
        return false;
    }

    const auto materialize_end = std::chrono::steady_clock::now();

    const std::size_t max_cells = MaxCells(request.state.Count());
    blitzar_trees::Octree tree(request.state.Count(), max_cells, 16, 32);
    const auto tree_begin = std::chrono::steady_clock::now();
    const blitzar_status tree_status = tree.Build(storage.View());
    const auto tree_end = std::chrono::steady_clock::now();
    const auto scan_begin = std::chrono::steady_clock::now();
    const double scan_checksum = storage.Scan();
    const auto scan_end = std::chrono::steady_clock::now();
    const std::uint64_t first_byte_hash = storage.ByteHash();
    const bool repeatable = storage.Load(request.state, request.order.Values()) &&
                            first_byte_hash == storage.ByteHash();

    const std::uint64_t scan_ns = Elapsed(scan_begin, scan_end);
    const bool tree_valid =
        tree_status == BLITZAR_STATUS_OK && tree.ParticleCount() == request.state.Count() &&
        tree.Indices().size() == request.state.Count() && tree.CellCount() <= max_cells;

    result.seed = seed_;
    result.particle_count = request.state.Count();
    result.ordering = request.ordering;
    result.layout = request.specification.kind;
    result.tile_width = request.specification.tile_width;
    result.sort_ns = request.sort_ns;
    result.materialize_ns = Elapsed(materialize_begin, materialize_end);
    result.tree_build_ns = Elapsed(tree_begin, tree_end);
    result.scan_ns = scan_ns;
    result.scan_particles_per_second =
        static_cast<double>(request.state.Count() * 7U) * 1.0e9 / static_cast<double>(scan_ns);

    result.locality_mean_squared_distance = request.order.Locality(request.state);
    result.cache_line_visits_proxy = storage.CacheLineVisits();
    result.candidate_bytes = storage.CandidateBytes();
    result.materialized_bytes = storage.MaterializedBytes();
    result.order_hash = request.order.Hash();
    result.state_hash = storage.LogicalHash();
    result.byte_hash = first_byte_hash;
    result.tree_hash = TreeHash(tree);
    result.scan_checksum = scan_checksum;
    result.stable = request.order.IsStable();
    result.repeatable = repeatable;
    result.ordering_equivalent = request.ordering_equivalent;
    result.tree_valid = tree_valid;

    return tree_valid && repeatable;
}

bool LayoutBenchmark::ValidateCount(std::span<LayoutResult> results) const noexcept
{
    if (results.size() != 10) {
        return false;
    }

    const std::uint64_t state_hash = results.front().state_hash;
    const std::uint64_t tree_hash = results.front().tree_hash;

    for (LayoutResult& result : results) {
        if (result.state_hash != state_hash || result.tree_hash != tree_hash || !result.stable ||
            !result.repeatable || !result.ordering_equivalent || !result.tree_valid) {
            return false;
        }

        result.representation_equivalent = true;
    }

    return true;
}

} // namespace blitzar_layout
