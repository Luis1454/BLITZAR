#include "neighborhood/NeighborBenchmark.hpp"

#include "neighborhood/NeighborCase.hpp"
#include "neighborhood/NeighborRun.hpp"
#include "neighborhood/NeighborTree.hpp"

#include <array>

namespace {

blitzar_neighborhood::NeighborResult MakeResult(
    const blitzar_neighborhood::NeighborWorkload& workload,
    blitzar_neighborhood::CandidateKind candidate, const blitzar_neighborhood::CandidateRun& run,
    const blitzar_neighborhood::TreeMetrics& tree)
{
    const auto& parameters = workload.parameters;

    return {workload.seed, workload.scenario, candidate, workload.positions.size(),
        parameters.steps, parameters.radius, parameters.skin, run.build_ns, run.query_ns,
        run.build_ns + run.query_ns, run.rebuild_count, run.neighbor_count, run.reference_count,
        run.memory_bytes, run.candidate_hash, run.reference_hash, run.ordering_hash, tree.build_ns,
        tree.cells, tree.memory_bytes, tree.hash, run.finite && tree.valid, run.correct,
        run.repeatable, candidate == blitzar_neighborhood::CandidateKind::CellLinked};
}

} // namespace

namespace blitzar_neighborhood {

NeighborBenchmark::NeighborBenchmark(std::uint64_t seed) : seed_(seed) {}

bool NeighborBenchmark::Run(std::vector<NeighborResult>& results) const
{
    results.clear();

    constexpr std::array<CandidateKind, 4> candidates{CandidateKind::CellLinked,
        CandidateKind::SpatialHash, CandidateKind::HilbertOrder, CandidateKind::Verlet};

    for (const NeighborWorkload& workload : MakeWorkloads(seed_)) {
        const TreeMetrics tree = MeasureTree(workload);

        for (const CandidateKind candidate : candidates) {
            results.push_back(
                MakeResult(workload, candidate, MeasureCandidate(workload, candidate), tree));
        }
    }

    return results.size() == 16U;
}

} // namespace blitzar_neighborhood
