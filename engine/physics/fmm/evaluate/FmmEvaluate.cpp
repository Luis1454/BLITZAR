/*
 * @file engine/physics/fmm/evaluate/FmmEvaluate.cpp
 * @brief Adaptive M2L, L2L, L2P and P2P passes for the CPU FMM solver.
 */

#include "physics/fmm/model/FmmInternal.hpp"
#include <cmath>

namespace bltzr_fmm {
struct Interaction final {
    int targetLevel;
    int targetIndex;
    int sourceLevel;
    int sourceIndex;
};

static bool isLeaf(const Cell& cell)
{
    return cell.children[0] < 0;
}

int degree(MultiIndex index)
{
    return index.x + index.y + index.z;
}

double softenedKernelDerivative(MultiIndex index, Vector3 displacement, float softening)
{
    const int order = degree(index);
    const std::array<double, 3> values{displacement.x, displacement.y, displacement.z};
    const double q = values[0] * values[0] + values[1] * values[1] + values[2] * values[2] +
                     static_cast<double>(softening) * static_cast<double>(softening);
    const double q3 = std::pow(q, 1.5);
    if (order == 0)
        return 1.0 / std::sqrt(q);
    std::array<int, 4> axes{};
    int cursor = 0;
    for (int axis = 0; axis < 3; ++axis) {
        const int count = axis == 0 ? index.x : (axis == 1 ? index.y : index.z);
        for (int copy = 0; copy < count; ++copy)
            axes[static_cast<std::size_t>(cursor++)] = axis;
    }
    if (order == 1)
        return -values[static_cast<std::size_t>(axes[0])] / q3;
    const auto delta = [&axes](int left, int right) {
        return axes[static_cast<std::size_t>(left)] == axes[static_cast<std::size_t>(right)] ? 1.0
                                                                                             : 0.0;
    };
    const double q5 = q3 * q;
    if (order == 2)
        return (3.0 * values[static_cast<std::size_t>(axes[0])] *
                    values[static_cast<std::size_t>(axes[1])] -
                delta(0, 1) * q) /
               q5;
    const double q7 = q5 * q;
    if (order == 3) {
        const double product = values[static_cast<std::size_t>(axes[0])] *
                               values[static_cast<std::size_t>(axes[1])] *
                               values[static_cast<std::size_t>(axes[2])];
        const double trace = delta(0, 1) * values[static_cast<std::size_t>(axes[2])] +
                             delta(0, 2) * values[static_cast<std::size_t>(axes[1])] +
                             delta(1, 2) * values[static_cast<std::size_t>(axes[0])];
        return (-15.0 * product + 3.0 * q * trace) / q7;
    }
    const double q9 = q7 * q;
    const double product =
        values[static_cast<std::size_t>(axes[0])] * values[static_cast<std::size_t>(axes[1])] *
        values[static_cast<std::size_t>(axes[2])] * values[static_cast<std::size_t>(axes[3])];
    const double pairs = delta(0, 1) * values[static_cast<std::size_t>(axes[2])] *
                             values[static_cast<std::size_t>(axes[3])] +
                         delta(0, 2) * values[static_cast<std::size_t>(axes[1])] *
                             values[static_cast<std::size_t>(axes[3])] +
                         delta(0, 3) * values[static_cast<std::size_t>(axes[1])] *
                             values[static_cast<std::size_t>(axes[2])] +
                         delta(1, 2) * values[static_cast<std::size_t>(axes[0])] *
                             values[static_cast<std::size_t>(axes[3])] +
                         delta(1, 3) * values[static_cast<std::size_t>(axes[0])] *
                             values[static_cast<std::size_t>(axes[2])] +
                         delta(2, 3) * values[static_cast<std::size_t>(axes[0])] *
                             values[static_cast<std::size_t>(axes[1])];
    const double doublePairs =
        delta(0, 1) * delta(2, 3) + delta(0, 2) * delta(1, 3) + delta(0, 3) * delta(1, 2);
    return (105.0 * product - 15.0 * q * pairs + 3.0 * q * q * doublePairs) / q9;
}

void translateMultipoleToLocal(const Cell& source, Vector3 sourceCenter, Cell& target,
                               Vector3 targetCenter, float softening)
{
    const Vector3 separation = targetCenter - sourceCenter;
    for (std::size_t local = 0; local < kIndices.size(); ++local) {
        for (std::size_t moment = 0; moment < kIndices.size(); ++moment) {
            const MultiIndex combined{kIndices[local].x + kIndices[moment].x,
                                      kIndices[local].y + kIndices[moment].y,
                                      kIndices[local].z + kIndices[moment].z};
            if (degree(combined) > 2 * kExpansionOrder)
                continue;
            const double sign = degree(kIndices[moment]) % 2 == 0 ? -1.0 : 1.0;
            target.local[local] += sign * source.multipole[moment] *
                                   softenedKernelDerivative(combined, separation, softening) /
                                   (factorial(kIndices[local]) * factorial(kIndices[moment]));
        }
    }
}

void propagateLocal(const Cell& parent, Vector3 parentCenter, Cell& child, Vector3 childCenter)
{
    const Vector3 displacement = childCenter - parentCenter;
    for (std::size_t lower = 0; lower < kIndices.size(); ++lower) {
        for (std::size_t upper = 0; upper < kIndices.size(); ++upper) {
            if (!isComponentwiseLessOrEqual(kIndices[lower], kIndices[upper]))
                continue;
            const MultiIndex delta{kIndices[upper].x - kIndices[lower].x,
                                   kIndices[upper].y - kIndices[lower].y,
                                   kIndices[upper].z - kIndices[lower].z};
            child.local[lower] += parent.local[upper] * binomial(kIndices[upper], kIndices[lower]) *
                                  monomial(displacement, delta);
        }
    }
}

Vector3 directAcceleration(Vector3 target, Vector3 source, float mass, const ForceLawPolicy& policy)
{
    const Vector3 delta = source - target;
    const float distanceSquared = dot(delta, delta) + policy.softening * policy.softening;
    if (distanceSquared <= policy.minDistance2)
        return Vector3();
    const float inverseDistance = 1.0f / std::sqrt(distanceSquared);
    return delta * (mass * inverseDistance * inverseDistance * inverseDistance);
}

bool acceptsMac(const FmmWorkspace& workspace, const Cell& target, const Cell& source)
{
    const Vector3 separation = cellCenter(workspace, target) - cellCenter(workspace, source);
    const float distance = std::sqrt(dot(separation, separation));
    const float targetHalf = workspace.sideLength / static_cast<float>(1 << (target.level + 1));
    const float sourceHalf = workspace.sideLength / static_cast<float>(1 << (source.level + 1));
    const float radius = 1.7320508f * (targetHalf + sourceHalf);
    return distance > 0.0f && radius / distance < workspace.parameters.theta;
}

bool finite(Vector3 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

void evaluateInteractions(const std::vector<Particle>& particles, const ForceLawPolicy& policy,
                          FmmWorkspace& workspace, std::vector<Vector3>& accelerations)
{
    accelerations.assign(particles.size(), Vector3());
    workspace.metrics.m2lInteractions = 0u;
    workspace.metrics.p2pInteractions = 0u;
    for (int level = 0; level <= workspace.depth; ++level) {
        for (Cell& cell : workspace.levels[static_cast<std::size_t>(level)].cells)
            cell.local.fill(0.0);
    }
    std::vector<Interaction> stack;
    stack.push_back({0, 0, 0, 0});
    while (!stack.empty()) {
        const Interaction current = stack.back();
        stack.pop_back();
        Cell& target = workspace.levels[static_cast<std::size_t>(current.targetLevel)]
                           .cells[static_cast<std::size_t>(current.targetIndex)];
        const Cell& source = workspace.levels[static_cast<std::size_t>(current.sourceLevel)]
                                 .cells[static_cast<std::size_t>(current.sourceIndex)];
        if (target.particleCount == 0 || source.particleCount == 0)
            continue;
        if (acceptsMac(workspace, target, source)) {
            translateMultipoleToLocal(source, cellCenter(workspace, source), target,
                                      cellCenter(workspace, target), policy.softening);
            ++workspace.metrics.m2lInteractions;
            continue;
        }
        const bool targetLeaf = isLeaf(target);
        const bool sourceLeaf = isLeaf(source);
        if (targetLeaf && sourceLeaf) {
            for (int targetOffset = 0; targetOffset < target.particleCount; ++targetOffset) {
                const int targetIndex = workspace.particleOrder[static_cast<std::size_t>(
                    target.firstParticle + targetOffset)];
                const Vector3 targetPosition =
                    particles[static_cast<std::size_t>(targetIndex)].getPosition();
                for (int sourceOffset = 0; sourceOffset < source.particleCount; ++sourceOffset) {
                    const int sourceIndex = workspace.particleOrder[static_cast<std::size_t>(
                        source.firstParticle + sourceOffset)];
                    if (sourceIndex != targetIndex) {
                        accelerations[static_cast<std::size_t>(targetIndex)] += directAcceleration(
                            targetPosition,
                            particles[static_cast<std::size_t>(sourceIndex)].getPosition(),
                            particles[static_cast<std::size_t>(sourceIndex)].getMass(), policy);
                        ++workspace.metrics.p2pInteractions;
                    }
                }
            }
            continue;
        }
        const bool splitTarget = !targetLeaf && (sourceLeaf || target.level <= source.level);
        if (splitTarget) {
            for (const int child : target.children)
                stack.push_back(
                    {current.targetLevel + 1, child, current.sourceLevel, current.sourceIndex});
        }
        else {
            for (const int child : source.children)
                stack.push_back(
                    {current.targetLevel, current.targetIndex, current.sourceLevel + 1, child});
        }
    }
    for (int level = 0; level < workspace.depth; ++level) {
        Level& parents = workspace.levels[static_cast<std::size_t>(level)];
        Level& children = workspace.levels[static_cast<std::size_t>(level + 1)];
        for (const Cell& parent : parents.cells) {
            if (isLeaf(parent))
                continue;
            for (const int child : parent.children)
                propagateLocal(
                    parent, cellCenter(workspace, parent),
                    children.cells[static_cast<std::size_t>(child)],
                    cellCenter(workspace, children.cells[static_cast<std::size_t>(child)]));
        }
    }
    workspace.metrics.finite = true;
    for (int level = 0; level <= workspace.depth; ++level) {
        const Level& leaves = workspace.levels[static_cast<std::size_t>(level)];
        for (const Cell& target : leaves.cells) {
            if (!isLeaf(target) || target.particleCount == 0)
                continue;
            const Vector3 center = cellCenter(workspace, target);
            for (int offset = 0; offset < target.particleCount; ++offset) {
                const int targetIndex =
                    workspace
                        .particleOrder[static_cast<std::size_t>(target.firstParticle + offset)];
                const Vector3 relative =
                    particles[static_cast<std::size_t>(targetIndex)].getPosition() - center;
                Vector3 acceleration = accelerations[static_cast<std::size_t>(targetIndex)];
                for (std::size_t coefficient = 1; coefficient < kIndices.size(); ++coefficient) {
                    const MultiIndex index = kIndices[coefficient];
                    const std::array<int, 3> exponents{index.x, index.y, index.z};
                    for (int axis = 0; axis < 3; ++axis) {
                        if (exponents[static_cast<std::size_t>(axis)] == 0)
                            continue;
                        MultiIndex derivative = index;
                        if (axis == 0)
                            --derivative.x;
                        else if (axis == 1)
                            --derivative.y;
                        else
                            --derivative.z;
                        const float contribution = static_cast<float>(
                            -target.local[coefficient] * exponents[static_cast<std::size_t>(axis)] *
                            monomial(relative, derivative));
                        if (axis == 0)
                            acceleration.x += contribution;
                        else if (axis == 1)
                            acceleration.y += contribution;
                        else
                            acceleration.z += contribution;
                    }
                }
                accelerations[static_cast<std::size_t>(targetIndex)] = acceleration;
                workspace.metrics.finite = workspace.metrics.finite && finite(acceleration);
            }
        }
    }
}

bool computeForces(const std::vector<Particle>& particles, const ForceLawPolicy& policy,
                   FmmWorkspace& workspace, std::vector<Vector3>& accelerations)
{
    if (!buildHierarchy(particles, workspace))
        return false;
    buildMultipoles(particles, workspace);
    evaluateInteractions(particles, policy, workspace, accelerations);
    return workspace.metrics.finite && accelerations.size() == particles.size();
}
} // namespace bltzr_fmm
