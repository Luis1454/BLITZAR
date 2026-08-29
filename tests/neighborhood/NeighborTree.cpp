#include "neighborhood/NeighborTree.hpp"

#include "neighborhood/NeighborCase.hpp"
#include "trees/octree/Octree.hpp"

#include <algorithm>
#include <bit>
#include <chrono>

namespace {

std::uint64_t Mix(std::uint64_t hash, std::uint64_t value) noexcept
{
    hash ^= value;
    hash *= 1099511628211ULL;

    return hash;
}

std::uint64_t HashTree(const blitzar_trees::Octree& tree) noexcept
{
    std::uint64_t hash = 1469598103934665603ULL;

    for (const auto& cell : tree.Cells()) {
        hash = Mix(hash, std::bit_cast<std::uint64_t>(cell.center.x));
        hash = Mix(hash, std::bit_cast<std::uint64_t>(cell.center.y));
        hash = Mix(hash, std::bit_cast<std::uint64_t>(cell.center.z));
        hash = Mix(hash, static_cast<std::uint64_t>(cell.begin));
        hash = Mix(hash, static_cast<std::uint64_t>(cell.count));
    }
    for (const std::size_t index : tree.Indices()) {
        hash = Mix(hash, static_cast<std::uint64_t>(index));
    }

    return hash;
}

std::uint64_t Elapsed(
    std::chrono::steady_clock::time_point begin, std::chrono::steady_clock::time_point end) noexcept
{
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();

    return static_cast<std::uint64_t>(elapsed > 0 ? elapsed : 1);
}

} // namespace

namespace blitzar_neighborhood {

TreeMetrics MeasureTree(const NeighborWorkload& workload)
{
    TreeMetrics metrics{0, 0, 0, 1469598103934665603ULL, true};
    blitzar_trees::Octree tree(
        workload.positions.size(), workload.positions.size() * 16U + 1U, 8U, 16U);

    for (std::size_t step = 0; step < workload.parameters.steps; ++step) {
        if (!IsMoving(workload.scenario) && step > 0) {
            continue;
        }

        const NeighborFrame frame = MakeFrame(workload, step);
        const auto begin = std::chrono::steady_clock::now();
        const blitzar_status status = tree.Build(frame.View());
        const auto end = std::chrono::steady_clock::now();

        metrics.build_ns += Elapsed(begin, end);

        if (status != BLITZAR_STATUS_OK) {
            metrics.valid = false;

            continue;
        }

        metrics.cells = std::max(metrics.cells, tree.CellCount());
        metrics.memory_bytes =
            std::max(metrics.memory_bytes, tree.MaxCells() * sizeof(blitzar_trees::Octree::Cell) +
                                               tree.MaxParticles() * sizeof(std::size_t));

        metrics.hash = Mix(metrics.hash, HashTree(tree));
    }

    return metrics;
}

} // namespace blitzar_neighborhood
