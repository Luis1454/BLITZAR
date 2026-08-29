#include "bvh/BvhBenchmark.hpp"
#include "fixtures/FixtureCheck.hpp"

#include <cstdio>
#include <string_view>
#include <vector>

namespace {

std::string_view ScenarioText(blitzar_neighborhood::ScenarioKind scenario) noexcept
{
    switch (scenario) {
    case blitzar_neighborhood::ScenarioKind::Dense:

        return "dense";

    case blitzar_neighborhood::ScenarioKind::Sparse:

        return "sparse";

    case blitzar_neighborhood::ScenarioKind::Clustered:

        return "clustered";

    case blitzar_neighborhood::ScenarioKind::Moving:

        return "moving";
    }

    return "unknown";
}

void PrintResult(const blitzar_bvh::BvhScenarioResult& result) noexcept
{
    const std::string_view scenario = ScenarioText(result.scenario);

    std::fprintf(stdout,
        "BLITZAR BVH schema=1 seed=%llu scenario=%.*s particles=%zu steps=%zu radius=%.17g "
        "skin=%.17g leaf_size=%zu bvh_build_ns=%llu bvh_refit_ns=%llu bvh_query_ns=%llu "
        "bvh_rebuild_ns=%llu cell_linked_build_ns=%llu cell_linked_query_ns=%llu "
        "bvh_rebuild_count=%zu bvh_refit_count=%zu bvh_rebuild_baseline_count=%zu "
        "bvh_neighbor_count=%zu cell_linked_neighbor_count=%zu reference_count=%zu "
        "bvh_memory_bytes=%zu bvh_workspace_bytes=%zu cell_linked_memory_bytes=%zu "
        "bvh_hash=%llu bvh_topology_hash=%llu cell_linked_hash=%llu reference_hash=%llu "
        "bvh_ordering_hash=%llu cell_linked_ordering_hash=%llu octree_build_ns=%llu "
        "octree_cells=%zu octree_memory_bytes=%zu octree_hash=%llu finite=%d correct=%d "
        "repeatable=%d deterministic=%d refit_correct=%d rebuild_correct=%d refit_parity=%d "
        "selected=%d decision=not-selected\n",
        static_cast<unsigned long long>(result.seed), static_cast<int>(scenario.size()),
        scenario.data(), result.particle_count, result.steps, result.radius, result.skin,
        result.leaf_size, static_cast<unsigned long long>(result.bvh_build_ns),
        static_cast<unsigned long long>(result.bvh_refit_ns),
        static_cast<unsigned long long>(result.bvh_query_ns),
        static_cast<unsigned long long>(result.bvh_rebuild_ns),
        static_cast<unsigned long long>(result.cell_linked_build_ns),
        static_cast<unsigned long long>(result.cell_linked_query_ns), result.bvh_rebuild_count,
        result.bvh_refit_count, result.bvh_rebuild_baseline_count, result.bvh_neighbor_count,
        result.cell_linked_neighbor_count, result.reference_count, result.bvh_memory_bytes,
        result.bvh_workspace_bytes, result.cell_linked_memory_bytes,
        static_cast<unsigned long long>(result.bvh_hash),
        static_cast<unsigned long long>(result.bvh_topology_hash),
        static_cast<unsigned long long>(result.cell_linked_hash),
        static_cast<unsigned long long>(result.reference_hash),
        static_cast<unsigned long long>(result.bvh_ordering_hash),
        static_cast<unsigned long long>(result.cell_linked_ordering_hash),
        static_cast<unsigned long long>(result.octree_build_ns), result.octree_cells,
        result.octree_memory_bytes, static_cast<unsigned long long>(result.octree_hash),
        result.finite ? 1 : 0, result.correct ? 1 : 0, result.repeatable ? 1 : 0,
        result.deterministic ? 1 : 0, result.refit_correct ? 1 : 0, result.rebuild_correct ? 1 : 0,
        result.refit_parity ? 1 : 0, result.selected ? 1 : 0);
}

bool ValidateResult(const blitzar_bvh::BvhScenarioResult& result) noexcept
{
    return result.seed == 424242 && result.particle_count == 96U && result.steps == 6U &&
           result.radius == 0.75 && result.skin == 0.4 && result.leaf_size == 4U &&
           result.bvh_build_ns > 0 && result.bvh_query_ns > 0 && result.bvh_rebuild_ns > 0 &&
           result.cell_linked_build_ns > 0 && result.cell_linked_query_ns > 0 &&
           result.bvh_rebuild_count == 1U &&
           result.bvh_refit_count ==
               (result.scenario == blitzar_neighborhood::ScenarioKind::Moving ? 5U : 0U) &&
           result.bvh_rebuild_baseline_count ==
               (result.scenario == blitzar_neighborhood::ScenarioKind::Moving ? 6U : 1U) &&
           result.bvh_neighbor_count == result.cell_linked_neighbor_count &&
           result.bvh_neighbor_count == result.reference_count && result.bvh_memory_bytes > 0 &&
           result.bvh_workspace_bytes > 0 && result.cell_linked_memory_bytes > 0 &&
           result.bvh_hash == result.cell_linked_hash && result.bvh_hash == result.reference_hash &&
           result.bvh_ordering_hash == result.cell_linked_ordering_hash &&
           result.octree_build_ns > 0 && result.octree_cells > 0 &&
           result.octree_memory_bytes > 0 && result.finite && result.correct && result.repeatable &&
           result.deterministic && result.refit_correct && result.rebuild_correct &&
           result.refit_parity && !result.selected;
}

} // namespace

int main()
{
    blitzar_bvh::BvhBenchmark benchmark(424242);
    std::vector<blitzar_bvh::BvhScenarioResult> results;

    BLITZAR_CHECK(benchmark.Run(results));
    BLITZAR_CHECK(results.size() == 4U);

    for (const blitzar_bvh::BvhScenarioResult& result : results) {
        BLITZAR_CHECK(ValidateResult(result));

        PrintResult(result);
    }

    return 0;
}
