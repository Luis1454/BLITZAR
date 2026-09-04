#include "bvh/BvhBenchmark.hpp"

#include "bvh/BvhIndex.hpp"
#include "neighborhood/NeighborCase.hpp"
#include "neighborhood/NeighborGrid.hpp"
#include "neighborhood/NeighborReference.hpp"
#include "neighborhood/NeighborTree.hpp"

#include <algorithm>
#include <chrono>

namespace {

struct PassState final {
    const blitzar_neighborhood::NeighborWorkload& workload;
    blitzar_bvh::BvhIndex bvh{blitzar_bvh::kLeafSize};
    blitzar_bvh::BvhWorkspace workspace{};
    blitzar_neighborhood::GridIndex cell_linked;
    blitzar_bvh::BvhPassResult result{};
    bool use_refit{};

    PassState(const blitzar_neighborhood::NeighborWorkload& input, bool refit)
        : workload(input),
          cell_linked(input.parameters, blitzar_neighborhood::GridKind::CellLinked),
          use_refit(refit)
    {
    }
};

std::uint64_t Mix(std::uint64_t hash, std::uint64_t value) noexcept
{
    hash ^= value;
    hash *= 1099511628211ULL;

    return hash;
}

std::uint64_t Elapsed(
    std::chrono::steady_clock::time_point begin, std::chrono::steady_clock::time_point end) noexcept
{
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();

    return static_cast<std::uint64_t>(elapsed > 0 ? elapsed : 1);
}

bool PrepareBvh(
    PassState& state, const blitzar_neighborhood::NeighborFrame& frame, bool rebuild, bool refit)
{
    if (!rebuild && !refit) {
        return true;
    }

    const auto begin = std::chrono::steady_clock::now();
    const bool valid = rebuild ? state.bvh.Build(frame) : state.bvh.Refit(frame);
    const auto end = std::chrono::steady_clock::now();
    const std::uint64_t elapsed = Elapsed(begin, end);

    if (rebuild) {
        state.result.build_ns += elapsed;
        state.result.rebuild_ns += elapsed;

        ++state.result.rebuild_count;

        if (!state.use_refit) {
            ++state.result.rebuild_baseline_count;
        }
    }
    else {
        state.result.refit_ns += elapsed;

        ++state.result.refit_count;
    }

    return valid;
}

bool PrepareCellLinked(PassState& state, const blitzar_neighborhood::NeighborFrame& frame)
{
    const auto begin = std::chrono::steady_clock::now();
    const bool valid = state.cell_linked.Build(frame);
    const auto end = std::chrono::steady_clock::now();

    state.result.cell_linked_build_ns += Elapsed(begin, end);

    return valid;
}

void Observe(PassState& state, const blitzar_neighborhood::NeighborFrame& frame)
{
    const auto bvh_begin = std::chrono::steady_clock::now();
    const blitzar_neighborhood::NeighborSet bvh_result =
        state.bvh.Query(frame, state.workload.parameters.radius, state.workspace);

    const auto bvh_end = std::chrono::steady_clock::now();
    const auto cell_begin = std::chrono::steady_clock::now();
    const blitzar_neighborhood::NeighborSet cell_result = state.cell_linked.Query(frame);
    const auto cell_end = std::chrono::steady_clock::now();
    const blitzar_neighborhood::NeighborSet reference =
        blitzar_neighborhood::BuildReference(frame, state.workload.parameters.radius);

    state.result.query_ns += Elapsed(bvh_begin, bvh_end);
    state.result.cell_linked_query_ns += Elapsed(cell_begin, cell_end);
    state.result.neighbor_count += bvh_result.Count();
    state.result.cell_linked_neighbor_count += cell_result.Count();
    state.result.reference_count += reference.Count();
    state.result.bvh_hash = Mix(state.result.bvh_hash, bvh_result.Hash());
    state.result.bvh_topology_hash = Mix(state.result.bvh_topology_hash, state.bvh.Hash());
    state.result.cell_linked_hash = Mix(state.result.cell_linked_hash, cell_result.Hash());
    state.result.reference_hash = Mix(state.result.reference_hash, reference.Hash());
    state.result.bvh_ordering_hash = Mix(state.result.bvh_ordering_hash, bvh_result.OrderingHash());
    state.result.cell_linked_ordering_hash =
        Mix(state.result.cell_linked_ordering_hash, cell_result.OrderingHash());

    const bool bvh_valid = bvh_result.IsValid(frame.x.size());
    const bool cell_valid = cell_result.IsValid(frame.x.size());
    const bool reference_valid = reference.IsValid(frame.x.size());

    state.result.finite = state.result.finite && bvh_valid && cell_valid && reference_valid;
    state.result.correct = state.result.correct && bvh_valid && reference_valid &&
                           blitzar_neighborhood::AreEqual(bvh_result, reference);

    state.result.cell_linked_correct = state.result.cell_linked_correct && cell_valid &&
                                       reference_valid &&
                                       blitzar_neighborhood::AreEqual(cell_result, reference);

    state.result.bvh_memory_bytes = std::max(state.result.bvh_memory_bytes,
        state.bvh.MemoryBytes() + state.workspace.MemoryBytes() + bvh_result.MemoryBytes());

    state.result.bvh_workspace_bytes =
        std::max(state.result.bvh_workspace_bytes, state.workspace.MemoryBytes());

    state.result.cell_linked_memory_bytes = std::max(state.result.cell_linked_memory_bytes,
        state.cell_linked.MemoryBytes() + cell_result.MemoryBytes());
}

bool RunStep(PassState& state, std::size_t step)
{
    const blitzar_neighborhood::NeighborFrame frame =
        blitzar_neighborhood::MakeFrame(state.workload, step);

    const bool moving = blitzar_neighborhood::IsMoving(state.workload.scenario);
    const bool rebuild = step == 0 || (!state.use_refit && moving);
    const bool refit = state.use_refit && moving && step > 0;
    const bool cell_rebuild = step == 0 || moving;

    if (!PrepareBvh(state, frame, rebuild, refit) ||
        (cell_rebuild && !PrepareCellLinked(state, frame))) {
        return false;
    }

    Observe(state, frame);

    return true;
}

bool RunPass(const blitzar_neighborhood::NeighborWorkload& workload, bool use_refit,
    blitzar_bvh::BvhPassResult& result)
{
    PassState state(workload, use_refit);

    for (std::size_t step = 0; step < workload.parameters.steps; ++step) {
        if (!RunStep(state, step)) {
            return false;
        }
    }

    result = state.result;

    return result.finite && result.correct && result.cell_linked_correct &&
           result.neighbor_count == result.reference_count &&
           result.cell_linked_neighbor_count == result.reference_count;
}

bool SameObservation(
    const blitzar_bvh::BvhPassResult& left, const blitzar_bvh::BvhPassResult& right) noexcept
{
    return left.rebuild_count == right.rebuild_count && left.refit_count == right.refit_count &&
           left.rebuild_baseline_count == right.rebuild_baseline_count &&
           left.neighbor_count == right.neighbor_count &&
           left.reference_count == right.reference_count &&
           left.cell_linked_neighbor_count == right.cell_linked_neighbor_count &&
           left.bvh_memory_bytes == right.bvh_memory_bytes &&
           left.bvh_workspace_bytes == right.bvh_workspace_bytes &&
           left.cell_linked_memory_bytes == right.cell_linked_memory_bytes &&
           left.bvh_hash == right.bvh_hash && left.bvh_topology_hash == right.bvh_topology_hash &&
           left.cell_linked_hash == right.cell_linked_hash &&
           left.reference_hash == right.reference_hash &&
           left.bvh_ordering_hash == right.bvh_ordering_hash &&
           left.cell_linked_ordering_hash == right.cell_linked_ordering_hash &&
           left.finite == right.finite && left.correct == right.correct &&
           left.cell_linked_correct == right.cell_linked_correct;
}

bool SameQuery(
    const blitzar_bvh::BvhPassResult& left, const blitzar_bvh::BvhPassResult& right) noexcept
{
    return left.neighbor_count == right.neighbor_count &&
           left.reference_count == right.reference_count &&
           left.cell_linked_neighbor_count == right.cell_linked_neighbor_count &&
           left.bvh_hash == right.bvh_hash && left.cell_linked_hash == right.cell_linked_hash &&
           left.reference_hash == right.reference_hash &&
           left.bvh_ordering_hash == right.bvh_ordering_hash &&
           left.cell_linked_ordering_hash == right.cell_linked_ordering_hash &&
           left.finite == right.finite && left.correct == right.correct &&
           left.cell_linked_correct == right.cell_linked_correct;
}

bool RunScenario(
    const blitzar_neighborhood::NeighborWorkload& workload, blitzar_bvh::BvhScenarioResult& result)
{
    blitzar_bvh::BvhPassResult refit_first{};
    blitzar_bvh::BvhPassResult refit_second{};
    blitzar_bvh::BvhPassResult rebuild{};

    if (!RunPass(workload, true, refit_first) || !RunPass(workload, true, refit_second) ||
        !RunPass(workload, false, rebuild)) {
        return false;
    }

    result = {};
    result.seed = workload.seed;
    result.scenario = workload.scenario;
    result.particle_count = workload.positions.size();
    result.steps = workload.parameters.steps;
    result.radius = workload.parameters.radius;
    result.skin = workload.parameters.skin;
    result.leaf_size = blitzar_bvh::kLeafSize;
    result.bvh_build_ns = refit_first.build_ns;
    result.bvh_refit_ns = refit_first.refit_ns;
    result.bvh_query_ns = refit_first.query_ns;
    result.bvh_rebuild_ns = rebuild.rebuild_ns;
    result.cell_linked_build_ns = refit_first.cell_linked_build_ns;
    result.cell_linked_query_ns = refit_first.cell_linked_query_ns;
    result.bvh_rebuild_count = refit_first.rebuild_count;
    result.bvh_refit_count = refit_first.refit_count;
    result.bvh_rebuild_baseline_count = rebuild.rebuild_baseline_count;
    result.bvh_neighbor_count = refit_first.neighbor_count;
    result.cell_linked_neighbor_count = refit_first.cell_linked_neighbor_count;
    result.reference_count = refit_first.reference_count;
    result.bvh_memory_bytes = refit_first.bvh_memory_bytes;
    result.bvh_workspace_bytes = refit_first.bvh_workspace_bytes;
    result.cell_linked_memory_bytes = refit_first.cell_linked_memory_bytes;
    result.bvh_hash = refit_first.bvh_hash;
    result.bvh_topology_hash = refit_first.bvh_topology_hash;
    result.cell_linked_hash = refit_first.cell_linked_hash;
    result.reference_hash = refit_first.reference_hash;
    result.bvh_ordering_hash = refit_first.bvh_ordering_hash;
    result.cell_linked_ordering_hash = refit_first.cell_linked_ordering_hash;

    const blitzar_neighborhood::TreeMetrics tree = blitzar_neighborhood::MeasureTree(workload);

    result.octree_build_ns = tree.build_ns;
    result.octree_cells = tree.cells;
    result.octree_memory_bytes = tree.memory_bytes;
    result.octree_hash = tree.hash;
    result.finite = refit_first.finite && refit_second.finite && rebuild.finite && tree.valid;
    result.refit_correct = refit_first.correct && refit_second.correct;
    result.rebuild_correct = rebuild.correct;
    result.correct = result.refit_correct && result.rebuild_correct;
    result.repeatable = SameObservation(refit_first, refit_second);
    result.deterministic = result.repeatable && SameQuery(refit_first, rebuild);
    result.refit_parity = SameQuery(refit_first, rebuild);
    result.selected = false;

    return result.finite && result.correct && result.repeatable && result.deterministic &&
           result.refit_parity;
}

} // namespace

namespace blitzar_bvh {

BvhBenchmark::BvhBenchmark(std::uint64_t seed) : seed_(seed) {}

bool BvhBenchmark::Run(std::vector<BvhScenarioResult>& results) const
{
    results.clear();
    results.reserve(4U);

    for (const blitzar_neighborhood::NeighborWorkload& workload :
        blitzar_neighborhood::MakeWorkloads(seed_)) {
        BvhScenarioResult result{};

        if (!RunScenario(workload, result)) {
            return false;
        }

        results.push_back(result);
    }

    return results.size() == 4U;
}

} // namespace blitzar_bvh
