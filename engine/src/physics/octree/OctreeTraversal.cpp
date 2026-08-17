/*
 * @file engine/src/physics/octree/OctreeTraversal.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief CPU octree force traversal.
 */

#include "OctreeForce.hpp"
#include "physics/octree/Octree.hpp"

#include <algorithm>
#include <cmath>

Vector3 Octree::computeForceRecursive(const std::vector<Particle>& particles, int nodeIndex,
                                      const Particle& particle, std::size_t selfIndex,
                                      const ForceLawPolicy& policy,
                                      OctreeOpeningCriterion criterion, float cutoffSquared) const
{
    if (nodeIndex < 0)
        return Vector3();

    Vector3 totalAcceleration;
    const Vector3 particlePosition = particle.getPosition();
    thread_local std::vector<int> traversalStack;
    traversalStack.clear();
    if (traversalStack.capacity() < 64u) {
        traversalStack.reserve(64u);
    }
    traversalStack.push_back(nodeIndex);

    while (!traversalStack.empty()) {
        const int currentNodeIndex = traversalStack.back();
        traversalStack.pop_back();

        if (currentNodeIndex < 0)
            continue;

        const Node& node = _nodes[currentNodeIndex];
        if (node.mass <= 0.0f)
            continue;
        if (cutoffSquared > 0.0f) {
            const float dx =
                std::max(std::fabs(particlePosition.x - node.center.x) - node.halfSize, 0.0f);
            const float dy =
                std::max(std::fabs(particlePosition.y - node.center.y) - node.halfSize, 0.0f);
            const float dz =
                std::max(std::fabs(particlePosition.z - node.center.z) - node.halfSize, 0.0f);
            if (dx * dx + dy * dy + dz * dz > cutoffSquared)
                continue;
        }

        if (!hasChildren(node)) {
            for (int particleIndex : node.particleIndices) {
                if (particleIndex == static_cast<int>(selfIndex))
                    continue;
                const Particle& other = particles[particleIndex];
                if (cutoffSquared > 0.0f) {
                    const Vector3 delta = other.getPosition() - particlePosition;
                    if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z > cutoffSquared)
                        continue;
                }
                totalAcceleration += blitzar_physics_particle_system_host::accelerationFromSource(
                    particlePosition, other.getPosition(), other.getMass(), policy);
            }
            continue;
        }

        const float size = node.halfSize * 2.0f;
        const bool containsSelf = std::fabs(particlePosition.x - node.center.x) <= node.halfSize &&
                                  std::fabs(particlePosition.y - node.center.y) <= node.halfSize &&
                                  std::fabs(particlePosition.z - node.center.z) <= node.halfSize;

        const Vector3 direction = node.centerOfMass - particlePosition;
        const float distance2 =
            blitzar_physics_particle_system_host::softenedDistanceSquared(direction, policy);
        float criterionDistance = std::max(std::sqrt(distance2), 1.0e-6f);

        if (criterion == OctreeOpeningCriterion::Bounds) {
            const float dx =
                std::max(std::fabs(particlePosition.x - node.center.x) - node.halfSize, 0.0f);
            const float dy =
                std::max(std::fabs(particlePosition.y - node.center.y) - node.halfSize, 0.0f);
            const float dz =
                std::max(std::fabs(particlePosition.z - node.center.z) - node.halfSize, 0.0f);
            criterionDistance = std::max(std::sqrt(dx * dx + dy * dy + dz * dz), 1.0e-6f);
        }

        const float maxDx = std::fabs(particlePosition.x - node.center.x) + node.halfSize;
        const float maxDy = std::fabs(particlePosition.y - node.center.y) + node.halfSize;
        const float maxDz = std::fabs(particlePosition.z - node.center.z) + node.halfSize;
        const bool insideCutoff =
            cutoffSquared <= 0.0f || maxDx * maxDx + maxDy * maxDy + maxDz * maxDz <= cutoffSquared;
        if (insideCutoff && !containsSelf && (size / criterionDistance) < policy.theta) {
            totalAcceleration += blitzar_physics_particle_system_host::accelerationFromSource(
                particlePosition, node.centerOfMass, node.mass, policy);
            continue;
        }

        for (int child = 0; child < 8; ++child) {
            if ((node.childMask & (1u << child)) != 0) {
                traversalStack.push_back(node.children[child]);
            }
        }
    }

    return totalAcceleration;
}

Vector3 Octree::computeForceRecursive(const std::vector<ParticleHotData>& particles, int nodeIndex,
                                      const ParticleHotData& particle, std::size_t selfIndex,
                                      const ForceLawPolicy& policy,
                                      OctreeOpeningCriterion criterion, float cutoffSquared) const
{
    if (nodeIndex < 0)
        return Vector3();

    Vector3 totalAcceleration;
    const Vector3 particlePosition = particle.getPosition();
    thread_local std::vector<int> traversalStack;
    traversalStack.clear();
    if (traversalStack.capacity() < 64u) {
        traversalStack.reserve(64u);
    }
    traversalStack.push_back(nodeIndex);

    while (!traversalStack.empty()) {
        const int currentNodeIndex = traversalStack.back();
        traversalStack.pop_back();

        if (currentNodeIndex < 0)
            continue;

        const Node& node = _nodes[currentNodeIndex];
        if (node.mass <= 0.0f)
            continue;
        if (cutoffSquared > 0.0f) {
            const float dx =
                std::max(std::fabs(particlePosition.x - node.center.x) - node.halfSize, 0.0f);
            const float dy =
                std::max(std::fabs(particlePosition.y - node.center.y) - node.halfSize, 0.0f);
            const float dz =
                std::max(std::fabs(particlePosition.z - node.center.z) - node.halfSize, 0.0f);
            if (dx * dx + dy * dy + dz * dz > cutoffSquared)
                continue;
        }

        if (!hasChildren(node)) {
            for (int particleIndex : node.particleIndices) {
                if (particleIndex == static_cast<int>(selfIndex))
                    continue;
                const ParticleHotData& other = particles[particleIndex];
                if (cutoffSquared > 0.0f) {
                    const Vector3 delta = other.getPosition() - particlePosition;
                    if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z > cutoffSquared)
                        continue;
                }
                totalAcceleration += blitzar_physics_particle_system_host::accelerationFromSource(
                    particlePosition, other.getPosition(), other.getMass(), policy);
            }
            continue;
        }

        const float size = node.halfSize * 2.0f;
        const bool containsSelf = std::fabs(particlePosition.x - node.center.x) <= node.halfSize &&
                                  std::fabs(particlePosition.y - node.center.y) <= node.halfSize &&
                                  std::fabs(particlePosition.z - node.center.z) <= node.halfSize;

        const Vector3 direction = node.centerOfMass - particlePosition;
        const float distance2 =
            blitzar_physics_particle_system_host::softenedDistanceSquared(direction, policy);
        float criterionDistance = std::max(std::sqrt(distance2), 1.0e-6f);

        if (criterion == OctreeOpeningCriterion::Bounds) {
            const float dx =
                std::max(std::fabs(particlePosition.x - node.center.x) - node.halfSize, 0.0f);
            const float dy =
                std::max(std::fabs(particlePosition.y - node.center.y) - node.halfSize, 0.0f);
            const float dz =
                std::max(std::fabs(particlePosition.z - node.center.z) - node.halfSize, 0.0f);
            criterionDistance = std::max(std::sqrt(dx * dx + dy * dy + dz * dz), 1.0e-6f);
        }

        const float maxDx = std::fabs(particlePosition.x - node.center.x) + node.halfSize;
        const float maxDy = std::fabs(particlePosition.y - node.center.y) + node.halfSize;
        const float maxDz = std::fabs(particlePosition.z - node.center.z) + node.halfSize;
        const bool insideCutoff =
            cutoffSquared <= 0.0f || maxDx * maxDx + maxDy * maxDy + maxDz * maxDz <= cutoffSquared;
        if (insideCutoff && !containsSelf && (size / criterionDistance) < policy.theta) {
            totalAcceleration += blitzar_physics_particle_system_host::accelerationFromSource(
                particlePosition, node.centerOfMass, node.mass, policy);
            continue;
        }

        for (int child = 0; child < 8; ++child) {
            if ((node.childMask & (1u << child)) != 0) {
                traversalStack.push_back(node.children[child]);
            }
        }
    }

    return totalAcceleration;
}

bool Octree::hasChildren(const Node& node)
{
    return node.childMask != 0u;
}
