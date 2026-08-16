/*
 * @file engine/src/physics/treepm/TreePmCpu.cpp
 * @brief CPU TreePM dispatch and precision-specific entry points.
 */

#include "physics/treepm/TreePmCpu.hpp"

#include <cmath>
#include <cstddef>
#include <functional>
#include <omp.h>
#include <optional>

namespace blitzar_physics_tree_pm_cpu {

// The force fragment depends on the grid, assignment, and field helpers.
// Keep this include order explicit across MSVC and Clang.
// clang-format off
#include "fragments/Math.inl"
#include "fragments/Assignment.inl"
#include "fragments/Field.inl"
#include "fragments/ShortRange.inl"
#include "fragments/Preparation.inl"
#include "fragments/Evaluation.inl"
#include "TreePmForce.inl"
// clang-format on

} // namespace blitzar_physics_tree_pm_cpu

bool computeCpuTreePmForces(const std::vector<Particle>& particles, const ForceLawPolicy& forceLaw,
                            const CpuTreePmParameters& parameters, CpuTreePmWorkspace& workspace,
                            Octree& shortRangeTree, OctreeOpeningCriterion openingCriterion,
                            std::vector<Vector3>& forces)
{
    return blitzar_physics_tree_pm_cpu::computeCpuTreePmForcesTyped<float>(
        particles, forceLaw, parameters, workspace, shortRangeTree, openingCriterion, forces,
        std::nullopt);
}

bool computeCpuTreePmForcesFp64(const std::vector<Particle>& particles,
                                const ForceLawPolicy& forceLaw,
                                const CpuTreePmParameters& parameters,
                                CpuTreePmFp64Workspace& workspace, Octree& shortRangeTree,
                                OctreeOpeningCriterion openingCriterion,
                                std::vector<Vector3>& forces)
{
    return blitzar_physics_tree_pm_cpu::computeCpuTreePmForcesTyped<double>(
        particles, forceLaw, parameters, workspace, shortRangeTree, openingCriterion, forces,
        std::nullopt);
}

bool computeCpuTreePmForcesSelective(const std::vector<Particle>& particles,
                                     const std::vector<int>& activeIndices,
                                     const ForceLawPolicy& forceLaw,
                                     const CpuTreePmParameters& parameters,
                                     CpuTreePmWorkspace& workspace, Octree& shortRangeTree,
                                     OctreeOpeningCriterion openingCriterion,
                                     std::vector<Vector3>& forces)
{
    return blitzar_physics_tree_pm_cpu::computeCpuTreePmForcesTyped<float>(
        particles, forceLaw, parameters, workspace, shortRangeTree, openingCriterion, forces,
        std::cref(activeIndices));
}

bool computeCpuTreePmForcesSelectiveFp64(const std::vector<Particle>& particles,
                                         const std::vector<int>& activeIndices,
                                         const ForceLawPolicy& forceLaw,
                                         const CpuTreePmParameters& parameters,
                                         CpuTreePmFp64Workspace& workspace, Octree& shortRangeTree,
                                         OctreeOpeningCriterion openingCriterion,
                                         std::vector<Vector3>& forces)
{
    return blitzar_physics_tree_pm_cpu::computeCpuTreePmForcesTyped<double>(
        particles, forceLaw, parameters, workspace, shortRangeTree, openingCriterion, forces,
        std::cref(activeIndices));
}

bool computeCpuFp64PairwiseForces(const std::vector<Particle>& particles,
                                  const ForceLawPolicy& forceLaw, std::vector<Vector3>& forces)
{
    if (particles.empty()) {
        return false;
    }
    const std::size_t count = particles.size();
    forces.assign(count, Vector3());
#pragma omp parallel for schedule(static)
    for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(count); ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        const Vector3 self = particles[index].getPosition();
        double forceX = 0.0;
        double forceY = 0.0;
        double forceZ = 0.0;
        for (std::size_t j = 0; j < count; ++j) {
            if (index == j) {
                continue;
            }
            const Vector3 delta = particles[j].getPosition() - self;
            const double distance2 = static_cast<double>(delta.x) * delta.x +
                                     static_cast<double>(delta.y) * delta.y +
                                     static_cast<double>(delta.z) * delta.z +
                                     static_cast<double>(forceLaw.softening) * forceLaw.softening;
            if (distance2 <= static_cast<double>(forceLaw.minDistance2)) {
                continue;
            }
            const double inverseDistance = 1.0 / std::sqrt(distance2);
            const double scale = static_cast<double>(particles[j].getMass()) * inverseDistance *
                                 inverseDistance * inverseDistance;
            forceX += static_cast<double>(delta.x) * scale;
            forceY += static_cast<double>(delta.y) * scale;
            forceZ += static_cast<double>(delta.z) * scale;
        }
        forces[index] = Vector3(static_cast<float>(forceX), static_cast<float>(forceY),
                                static_cast<float>(forceZ));
    }
    return true;
}
