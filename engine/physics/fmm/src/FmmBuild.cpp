/*
 * @file engine/physics/fmm/src/FmmBuild.cpp
 * @brief Adaptive hierarchy and multipole construction for the CPU FMM solver.
 */

#include "FmmInternal.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace bltzr_fmm {
struct CellReference final {
    int level;
    int index;
};

Cell makeCell(int x, int y, int z, int level, int parent, int firstParticle, int particleCount)
{
    Cell cell;
    cell.x = x;
    cell.y = y;
    cell.z = z;
    cell.level = level;
    cell.parent = parent;
    cell.firstParticle = firstParticle;
    cell.particleCount = particleCount;
    cell.children.fill(-1);
    return cell;
}

static bool isLeaf(const Cell& cell)
{
    return cell.children[0] < 0;
}

int chooseOctant(Vector3 position, Vector3 center)
{
    return (position.x >= center.x ? 1 : 0) | (position.y >= center.y ? 2 : 0) |
           (position.z >= center.z ? 4 : 0);
}

bool splitCell(const std::vector<Particle>& particles, FmmWorkspace& workspace, int level,
               int cellIndex)
{
    if (level >= workspace.parameters.maxDepth)
        return false;
    Cell parent = workspace.levels[static_cast<std::size_t>(level)]
                      .cells[static_cast<std::size_t>(cellIndex)];
    if (!isLeaf(parent))
        return false;

    const Vector3 center = cellCenter(workspace, parent);
    std::array<int, 8> counts{};
    for (int offset = 0; offset < parent.particleCount; ++offset) {
        const int particleIndex =
            workspace.particleOrder[static_cast<std::size_t>(parent.firstParticle + offset)];
        ++counts[static_cast<std::size_t>(chooseOctant(
            particles[static_cast<std::size_t>(particleIndex)].getPosition(), center))];
    }
    std::array<int, 8> starts{};
    std::array<int, 8> cursors{};
    for (int octant = 1; octant < 8; ++octant)
        starts[static_cast<std::size_t>(octant)] = starts[static_cast<std::size_t>(octant - 1)] +
                                                   counts[static_cast<std::size_t>(octant - 1)];
    cursors = starts;
    for (int offset = 0; offset < parent.particleCount; ++offset) {
        const int source =
            workspace.particleOrder[static_cast<std::size_t>(parent.firstParticle + offset)];
        const int octant =
            chooseOctant(particles[static_cast<std::size_t>(source)].getPosition(), center);
        workspace.reorderScratch[static_cast<std::size_t>(
            parent.firstParticle + cursors[static_cast<std::size_t>(octant)]++)] = source;
    }
    for (int offset = 0; offset < parent.particleCount; ++offset)
        workspace.particleOrder[static_cast<std::size_t>(parent.firstParticle + offset)] =
            workspace.reorderScratch[static_cast<std::size_t>(parent.firstParticle + offset)];

    Level& children = workspace.levels[static_cast<std::size_t>(level + 1)];
    Cell& updatedParent = workspace.levels[static_cast<std::size_t>(level)]
                              .cells[static_cast<std::size_t>(cellIndex)];
    for (int octant = 0; octant < 8; ++octant) {
        const int childIndex = static_cast<int>(children.cells.size());
        const int childX = parent.x * 2 + (octant & 1);
        const int childY = parent.y * 2 + ((octant >> 1) & 1);
        const int childZ = parent.z * 2 + ((octant >> 2) & 1);
        children.cells.push_back(
            makeCell(childX, childY, childZ, level + 1, cellIndex,
                     parent.firstParticle + starts[static_cast<std::size_t>(octant)],
                     counts[static_cast<std::size_t>(octant)]));
        updatedParent.children[static_cast<std::size_t>(octant)] = childIndex;
    }
    workspace.depth = std::max(workspace.depth, level + 1);
    return true;
}

std::vector<CellReference> collectLeaves(const FmmWorkspace& workspace)
{
    std::vector<CellReference> leaves;
    for (int level = 0; level <= workspace.depth; ++level) {
        const Level& current = workspace.levels[static_cast<std::size_t>(level)];
        for (std::size_t index = 0; index < current.cells.size(); ++index) {
            if (isLeaf(current.cells[index]))
                leaves.push_back({level, static_cast<int>(index)});
        }
    }
    return leaves;
}

bool cellsTouch(const Cell& left, const Cell& right)
{
    const int commonLevel = std::max(left.level, right.level);
    const int leftScale = 1 << (commonLevel - left.level);
    const int rightScale = 1 << (commonLevel - right.level);
    const auto overlaps = [](int minLeft, int maxLeft, int minRight, int maxRight) {
        return minLeft <= maxRight && minRight <= maxLeft;
    };
    return overlaps(left.x * leftScale, (left.x + 1) * leftScale, right.x * rightScale,
                    (right.x + 1) * rightScale) &&
           overlaps(left.y * leftScale, (left.y + 1) * leftScale, right.y * rightScale,
                    (right.y + 1) * rightScale) &&
           overlaps(left.z * leftScale, (left.z + 1) * leftScale, right.z * rightScale,
                    (right.z + 1) * rightScale);
}

void balanceTree(const std::vector<Particle>& particles, FmmWorkspace& workspace)
{
    workspace.metrics.balancedTwoToOne = true;
    for (;;) {
        const std::vector<CellReference> leaves = collectLeaves(workspace);
        bool split = false;
        for (std::size_t leftIndex = 0; leftIndex < leaves.size() && !split; ++leftIndex) {
            const CellReference leftRef = leaves[leftIndex];
            const Cell& left = workspace.levels[static_cast<std::size_t>(leftRef.level)]
                                   .cells[static_cast<std::size_t>(leftRef.index)];
            for (std::size_t rightIndex = leftIndex + 1; rightIndex < leaves.size(); ++rightIndex) {
                const CellReference rightRef = leaves[rightIndex];
                const Cell& right = workspace.levels[static_cast<std::size_t>(rightRef.level)]
                                        .cells[static_cast<std::size_t>(rightRef.index)];
                if (std::abs(left.level - right.level) <= 1 || !cellsTouch(left, right))
                    continue;
                const CellReference coarse = left.level < right.level ? leftRef : rightRef;
                if (!splitCell(particles, workspace, coarse.level, coarse.index)) {
                    workspace.metrics.balancedTwoToOne = false;
                }
                split = true;
                break;
            }
        }
        if (!split)
            return;
    }
}

double factorialInteger(int value)
{
    return value < 2 ? 1.0 : 2.0;
}

void configure(FmmWorkspace& workspace, int leafCapacity, float theta)
{
    workspace.parameters.leafCapacity = std::clamp(leafCapacity, 1, 1024);
    workspace.parameters.theta = std::clamp(theta, 0.1f, 1.0f);
}

std::uint64_t cellKey(int x, int y, int z)
{
    return static_cast<std::uint64_t>(x) | (static_cast<std::uint64_t>(y) << 20u) |
           (static_cast<std::uint64_t>(z) << 40u);
}

Vector3 cellCenter(const FmmWorkspace& workspace, const Cell& cell)
{
    const float width = workspace.sideLength / static_cast<float>(1 << cell.level);
    return workspace.origin + Vector3((static_cast<float>(cell.x) + 0.5f) * width,
                                      (static_cast<float>(cell.y) + 0.5f) * width,
                                      (static_cast<float>(cell.z) + 0.5f) * width);
}

double monomial(Vector3 value, MultiIndex index)
{
    return std::pow(static_cast<double>(value.x), index.x) *
           std::pow(static_cast<double>(value.y), index.y) *
           std::pow(static_cast<double>(value.z), index.z);
}

double factorial(MultiIndex index)
{
    return factorialInteger(index.x) * factorialInteger(index.y) * factorialInteger(index.z);
}

double binomial(MultiIndex upper, MultiIndex lower)
{
    const auto choose = [](int n, int k) {
        return n == 2 && k == 1 ? 2.0 : 1.0;
    };
    return choose(upper.x, lower.x) * choose(upper.y, lower.y) * choose(upper.z, lower.z);
}

bool isComponentwiseLessOrEqual(MultiIndex lower, MultiIndex upper)
{
    return lower.x <= upper.x && lower.y <= upper.y && lower.z <= upper.z;
}

bool buildHierarchy(const std::vector<Particle>& particles, FmmWorkspace& workspace)
{
    if (particles.empty())
        return false;
    Vector3 minimum = particles.front().getPosition();
    Vector3 maximum = minimum;
    for (const Particle& particle : particles) {
        const Vector3 position = particle.getPosition();
        minimum.x = std::min(minimum.x, position.x);
        minimum.y = std::min(minimum.y, position.y);
        minimum.z = std::min(minimum.z, position.z);
        maximum.x = std::max(maximum.x, position.x);
        maximum.y = std::max(maximum.y, position.y);
        maximum.z = std::max(maximum.z, position.z);
    }
    workspace.sideLength =
        std::max({maximum.x - minimum.x, maximum.y - minimum.y, maximum.z - minimum.z, 1.0e-4f}) *
        1.0001f;
    workspace.origin =
        (minimum + maximum) * 0.5f -
        Vector3(workspace.sideLength, workspace.sideLength, workspace.sideLength) * 0.5f;
    workspace.depth = 0;
    workspace.metrics = Metrics{};
    workspace.levels.assign(static_cast<std::size_t>(workspace.parameters.maxDepth + 1), Level{});
    workspace.particleOrder.resize(particles.size());
    workspace.reorderScratch.resize(particles.size());
    std::iota(workspace.particleOrder.begin(), workspace.particleOrder.end(), 0);
    workspace.levels.front().cells.push_back(
        makeCell(0, 0, 0, 0, -1, 0, static_cast<int>(particles.size())));
    for (int level = 0; level < workspace.parameters.maxDepth; ++level) {
        Level& current = workspace.levels[static_cast<std::size_t>(level)];
        const std::size_t cellCount = current.cells.size();
        for (std::size_t index = 0; index < cellCount; ++index) {
            if (current.cells[index].particleCount > workspace.parameters.leafCapacity)
                (void)splitCell(particles, workspace, level, static_cast<int>(index));
        }
    }
    balanceTree(particles, workspace);
    const std::vector<CellReference> leaves = collectLeaves(workspace);
    workspace.metrics.leafCount = static_cast<std::uint32_t>(leaves.size());
    workspace.metrics.minLeafDepth = static_cast<std::uint32_t>(workspace.depth);
    for (const CellReference ref : leaves) {
        workspace.metrics.minLeafDepth =
            std::min(workspace.metrics.minLeafDepth, static_cast<std::uint32_t>(ref.level));
        workspace.metrics.maxLeafDepth =
            std::max(workspace.metrics.maxLeafDepth, static_cast<std::uint32_t>(ref.level));
    }
    return true;
}

void buildMultipoles(const std::vector<Particle>& particles, FmmWorkspace& workspace)
{
    for (int level = workspace.depth; level >= 0; --level) {
        Level& current = workspace.levels[static_cast<std::size_t>(level)];
        for (Cell& cell : current.cells) {
            cell.multipole.fill(0.0);
            const Vector3 center = cellCenter(workspace, cell);
            if (isLeaf(cell)) {
                for (int offset = 0; offset < cell.particleCount; ++offset) {
                    const Particle& particle = particles[static_cast<std::size_t>(
                        workspace
                            .particleOrder[static_cast<std::size_t>(cell.firstParticle + offset)])];
                    const Vector3 relative = particle.getPosition() - center;
                    for (std::size_t coefficient = 0; coefficient < kIndices.size(); ++coefficient)
                        cell.multipole[coefficient] +=
                            particle.getMass() * monomial(relative, kIndices[coefficient]);
                }
                continue;
            }
            const Level& children = workspace.levels[static_cast<std::size_t>(level + 1)];
            for (const int childIndex : cell.children) {
                const Cell& child = children.cells[static_cast<std::size_t>(childIndex)];
                const Vector3 displacement = cellCenter(workspace, child) - center;
                for (std::size_t upper = 0; upper < kIndices.size(); ++upper) {
                    for (std::size_t lower = 0; lower < kIndices.size(); ++lower) {
                        if (!isComponentwiseLessOrEqual(kIndices[lower], kIndices[upper]))
                            continue;
                        const MultiIndex delta{kIndices[upper].x - kIndices[lower].x,
                                               kIndices[upper].y - kIndices[lower].y,
                                               kIndices[upper].z - kIndices[lower].z};
                        cell.multipole[upper] += child.multipole[lower] *
                                                 binomial(kIndices[upper], kIndices[lower]) *
                                                 monomial(displacement, delta);
                    }
                }
            }
        }
    }
}
} // namespace bltzr_fmm
