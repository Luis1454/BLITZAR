#include "fixtures/FixtureCheck.hpp"
#include "neighborhood/NeighborBenchmark.hpp"
#include "neighborhood/NeighborBoundary.hpp"
#include "neighborhood/NeighborCase.hpp"

#include <cmath>
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

std::string_view CandidateText(blitzar_neighborhood::CandidateKind candidate) noexcept
{
    switch (candidate) {
    case blitzar_neighborhood::CandidateKind::CellLinked:

        return "cell-linked-v1";

    case blitzar_neighborhood::CandidateKind::SpatialHash:

        return "spatial-hash-v1";

    case blitzar_neighborhood::CandidateKind::HilbertOrder:

        return "hilbert-order-v1";

    case blitzar_neighborhood::CandidateKind::Verlet:

        return "verlet-list-v1";
    }

    return "unknown";
}

std::size_t ExpectedRebuild(const blitzar_neighborhood::NeighborResult& result) noexcept
{
    return result.scenario == blitzar_neighborhood::ScenarioKind::Moving ? result.steps : 1U;
}

void PrintResult(const blitzar_neighborhood::NeighborResult& result) noexcept
{
    const std::string_view scenario = ScenarioText(result.scenario);
    const std::string_view candidate = CandidateText(result.candidate);

    std::fprintf(stdout,
        "BLITZAR NEIGHBOR schema=1 seed=%llu scenario=%.*s candidate=%.*s particles=%zu "
        "steps=%zu radius=%.17g skin=%.17g build_ns=%llu query_ns=%llu total_ns=%llu "
        "rebuild_count=%zu neighbor_count=%zu reference_count=%zu memory_bytes=%zu "
        "candidate_hash=%llu reference_hash=%llu ordering_hash=%llu octree_build_ns=%llu "
        "octree_cells=%zu octree_memory_bytes=%zu octree_hash=%llu finite=%d correct=%d "
        "repeatable=%d selected=%d\n",
        static_cast<unsigned long long>(result.seed), static_cast<int>(scenario.size()),
        scenario.data(), static_cast<int>(candidate.size()), candidate.data(),
        result.particle_count, result.steps, result.radius, result.skin,
        static_cast<unsigned long long>(result.build_ns),
        static_cast<unsigned long long>(result.query_ns),
        static_cast<unsigned long long>(result.total_ns), result.rebuild_count,
        result.neighbor_count, result.reference_count, result.memory_bytes,
        static_cast<unsigned long long>(result.candidate_hash),
        static_cast<unsigned long long>(result.reference_hash),
        static_cast<unsigned long long>(result.ordering_hash),
        static_cast<unsigned long long>(result.octree_build_ns), result.octree_cells,
        result.octree_memory_bytes, static_cast<unsigned long long>(result.octree_hash),
        result.finite ? 1 : 0, result.correct ? 1 : 0, result.repeatable ? 1 : 0,
        result.selected ? 1 : 0);
}

} // namespace

int main()
{
    constexpr std::uint64_t seed = 424242;

    BLITZAR_CHECK(blitzar_neighborhood::CheckBoundary());

    blitzar_neighborhood::NeighborBenchmark benchmark(seed);
    std::vector<blitzar_neighborhood::NeighborResult> results;

    BLITZAR_CHECK(benchmark.Run(results));
    BLITZAR_CHECK(results.size() == 16U);

    for (const auto& result : results) {
        BLITZAR_CHECK(
            result.seed == seed && result.particle_count == blitzar_neighborhood::kParticleCount);

        BLITZAR_CHECK(result.steps == blitzar_neighborhood::kStepCount && result.radius == 0.75 &&
                      result.skin == 0.4);

        BLITZAR_CHECK(result.build_ns > 0 && result.query_ns > 0 && result.total_ns > 0);
        BLITZAR_CHECK(result.rebuild_count == ExpectedRebuild(result));
        BLITZAR_CHECK(result.neighbor_count == result.reference_count);
        BLITZAR_CHECK(result.memory_bytes > 0 && result.octree_build_ns > 0);
        BLITZAR_CHECK(result.octree_cells > 0 && result.octree_memory_bytes > 0);
        BLITZAR_CHECK(result.finite && result.correct && result.repeatable);
        BLITZAR_CHECK(result.candidate_hash == result.reference_hash);
        BLITZAR_CHECK(result.selected ==
                      (result.candidate == blitzar_neighborhood::CandidateKind::CellLinked));

        PrintResult(result);
    }

    return 0;
}
