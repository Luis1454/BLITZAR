/*
 * @file engine/src/physics/octree/Octree.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief CPU octree construction, traversal, and GPU export.
 */

#include "physics/octree/Octree.hpp"
#include "OctreeForce.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <numeric>

namespace blitzar_physics_octree {
constexpr std::size_t kLeafCapacity = 16u;
constexpr int kMaxDepth = 24;
constexpr float kMinHalfSize = 0.01f;
} // namespace blitzar_physics_octree

Octree::Node::Node()
    : center(),
      halfSize(0.0f),
      mass(0.0f),
      centerOfMass(),
      children(),
      childMask(0u),
      particleIndices()
{
    children.fill(-1);
}

Octree::Octree() : _nodes(), _particlesRef(std::nullopt), _root(-1)
{
}

Octree::~Octree() = default;

void Octree::clear()
{
    _nodes.clear();
    _particlesRef.reset();
    _root = -1;
}

void Octree::build(const std::vector<Particle>& particles)
{
    clear();
    _particlesRef = std::cref(particles);
    if (particles.empty())
        return;

    _nodes.reserve(particles.size() * 2u);
    Vector3 minPosition = particles.front().getPosition();
    Vector3 maxPosition = minPosition;
    for (std::size_t i = 1; i < particles.size(); ++i) {
        const Vector3 position = particles[i].getPosition();
        minPosition.x = std::min(minPosition.x, position.x);
        minPosition.y = std::min(minPosition.y, position.y);
        minPosition.z = std::min(minPosition.z, position.z);
        maxPosition.x = std::max(maxPosition.x, position.x);
        maxPosition.y = std::max(maxPosition.y, position.y);
        maxPosition.z = std::max(maxPosition.z, position.z);
    }

    const Vector3 center((minPosition.x + maxPosition.x) * 0.5f,
                         (minPosition.y + maxPosition.y) * 0.5f,
                         (minPosition.z + maxPosition.z) * 0.5f);
    const float sizeX = maxPosition.x - minPosition.x;
    const float sizeY = maxPosition.y - minPosition.y;
    const float sizeZ = maxPosition.z - minPosition.z;
    const float halfSize = std::max(0.5f * std::max(sizeX, std::max(sizeY, sizeZ)),
                                    blitzar_physics_octree::kMinHalfSize) +
                           0.001f;

    std::vector<int> rootIndices(particles.size());
    std::iota(rootIndices.begin(), rootIndices.end(), 0);
    _root = buildNodeRecursive(particles, rootIndices, center, halfSize, 0);
}

void Octree::build(const std::vector<ParticleHotData>& particles)
{
    clear();
    if (particles.empty())
        return;

    _nodes.reserve(particles.size() * 2u);
    Vector3 minPosition = particles.front().getPosition();
    Vector3 maxPosition = minPosition;
    for (std::size_t i = 1; i < particles.size(); ++i) {
        const Vector3 position = particles[i].getPosition();
        minPosition.x = std::min(minPosition.x, position.x);
        minPosition.y = std::min(minPosition.y, position.y);
        minPosition.z = std::min(minPosition.z, position.z);
        maxPosition.x = std::max(maxPosition.x, position.x);
        maxPosition.y = std::max(maxPosition.y, position.y);
        maxPosition.z = std::max(maxPosition.z, position.z);
    }

    const Vector3 center((minPosition.x + maxPosition.x) * 0.5f,
                         (minPosition.y + maxPosition.y) * 0.5f,
                         (minPosition.z + maxPosition.z) * 0.5f);
    const float sizeX = maxPosition.x - minPosition.x;
    const float sizeY = maxPosition.y - minPosition.y;
    const float sizeZ = maxPosition.z - minPosition.z;
    const float halfSize = std::max(0.5f * std::max(sizeX, std::max(sizeY, sizeZ)),
                                    blitzar_physics_octree::kMinHalfSize) +
                           0.001f;

    std::vector<int> rootIndices(particles.size());
    std::iota(rootIndices.begin(), rootIndices.end(), 0);
    _root = buildNodeRecursive(particles, rootIndices, center, halfSize, 0);
}

Vector3 Octree::computeForceOn(const Particle& particle, std::size_t selfIndex,
                               const ForceLawPolicy& policy, OctreeOpeningCriterion criterion,
                               float cutoffSquared) const
{
    if (_root < 0 || !_particlesRef.has_value())
        return Vector3();
    return computeForceRecursive(_particlesRef->get(), _root, particle, selfIndex, policy,
                                 criterion, cutoffSquared);
}

Vector3 Octree::computeForceOn(const std::vector<ParticleHotData>& particles, std::size_t selfIndex,
                               const ForceLawPolicy& policy, OctreeOpeningCriterion criterion,
                               float cutoffSquared) const
{
    if (_root < 0 || particles.empty() || selfIndex >= particles.size())
        return Vector3();
    return computeForceRecursive(particles, _root, particles[selfIndex], selfIndex, policy,
                                 criterion, cutoffSquared);
}

std::size_t Octree::getNodeCount() const
{
    return _nodes.size();
}

int Octree::getRootIndex() const
{
    return _root;
}

void Octree::exportGpu(std::vector<GpuOctreeNode>& outNodes, std::vector<int>& outLeafIndices) const
{
    outNodes.clear();
    outLeafIndices.clear();
    outNodes.resize(_nodes.size());
    std::size_t totalLeafIndices = 0u;
    for (const Node& node : _nodes) {
        totalLeafIndices += node.particleIndices.size();
    }
    outLeafIndices.reserve(totalLeafIndices);

    for (std::size_t i = 0; i < _nodes.size(); ++i) {
        const Node& source = _nodes[i];
        GpuOctreeNode destination{};
        destination.centerX = source.center.x;
        destination.centerY = source.center.y;
        destination.centerZ = source.center.z;
        destination.halfSize = source.halfSize;
        destination.mass = source.mass;
        destination.comX = source.centerOfMass.x;
        destination.comY = source.centerOfMass.y;
        destination.comZ = source.centerOfMass.z;
        for (int child = 0; child < 8; ++child) {
            destination.children[child] = source.children[child];
        }
        destination.childMask = source.childMask;
        destination.leafStart = static_cast<int>(outLeafIndices.size());
        destination.leafCount = static_cast<int>(source.particleIndices.size());
        destination.parentIndex = -1;
        destination.nextIndex = -1;
        for (int leafIndex : source.particleIndices) {
            outLeafIndices.push_back(leafIndex);
        }
        outNodes[i] = destination;
    }
}

int Octree::buildNodeRecursive(const std::vector<Particle>& particles,
                               const std::vector<int>& indices, const Vector3& center,
                               float halfSize, int depth)
{
    Node node;
    node.center = center;
    node.halfSize = halfSize;

    Vector3 weightedCenter;
    float totalMass = 0.0f;
    for (int particleIndex : indices) {
        const Particle& particle = particles[particleIndex];
        const float mass = particle.getMass();
        totalMass += mass;
        weightedCenter += particle.getPosition() * mass;
    }
    node.mass = totalMass;
    node.centerOfMass = totalMass > 0.0f ? weightedCenter / totalMass : center;

    const int nodeIndex = static_cast<int>(_nodes.size());
    _nodes.push_back(node);
    if (indices.size() <= blitzar_physics_octree::kLeafCapacity ||
        halfSize <= blitzar_physics_octree::kMinHalfSize ||
        depth >= blitzar_physics_octree::kMaxDepth) {
        _nodes[nodeIndex].particleIndices = indices;
        return nodeIndex;
    }

    std::array<std::vector<int>, 8> buckets;
    for (std::vector<int>& bucket : buckets) {
        bucket.reserve(indices.size() / 4u + 1u);
    }
    int nonEmptyBuckets = 0;
    for (int particleIndex : indices) {
        const int child = childIndexForPosition(particles[particleIndex].getPosition(), center);
        if (buckets[child].empty()) {
            ++nonEmptyBuckets;
        }
        buckets[child].push_back(particleIndex);
    }
    if (nonEmptyBuckets <= 1) {
        _nodes[nodeIndex].particleIndices = indices;
        return nodeIndex;
    }

    const float childHalfSize = halfSize * 0.5f;
    for (int child = 0; child < 8; ++child) {
        if (buckets[child].empty())
            continue;
        const Vector3 childCenter(center.x + ((child & 1) != 0 ? childHalfSize : -childHalfSize),
                                  center.y + ((child & 2) != 0 ? childHalfSize : -childHalfSize),
                                  center.z + ((child & 4) != 0 ? childHalfSize : -childHalfSize));
        _nodes[nodeIndex].children[child] =
            buildNodeRecursive(particles, buckets[child], childCenter, childHalfSize, depth + 1);
        _nodes[nodeIndex].childMask |= static_cast<unsigned char>(1u << child);
    }
    return nodeIndex;
}

int Octree::buildNodeRecursive(const std::vector<ParticleHotData>& particles,
                               const std::vector<int>& indices, const Vector3& center,
                               float halfSize, int depth)
{
    Node node;
    node.center = center;
    node.halfSize = halfSize;

    Vector3 weightedCenter;
    float totalMass = 0.0f;
    for (int particleIndex : indices) {
        const ParticleHotData& particle = particles[particleIndex];
        const float mass = particle.getMass();
        totalMass += mass;
        weightedCenter += particle.getPosition() * mass;
    }
    node.mass = totalMass;
    node.centerOfMass = totalMass > 0.0f ? weightedCenter / totalMass : center;

    const int nodeIndex = static_cast<int>(_nodes.size());
    _nodes.push_back(node);
    if (indices.size() <= blitzar_physics_octree::kLeafCapacity ||
        halfSize <= blitzar_physics_octree::kMinHalfSize ||
        depth >= blitzar_physics_octree::kMaxDepth) {
        _nodes[nodeIndex].particleIndices = indices;
        return nodeIndex;
    }

    std::array<std::vector<int>, 8> buckets;
    for (std::vector<int>& bucket : buckets) {
        bucket.reserve(indices.size() / 4u + 1u);
    }
    int nonEmptyBuckets = 0;
    for (int particleIndex : indices) {
        const int child = childIndexForPosition(particles[particleIndex].getPosition(), center);
        if (buckets[child].empty()) {
            ++nonEmptyBuckets;
        }
        buckets[child].push_back(particleIndex);
    }
    if (nonEmptyBuckets <= 1) {
        _nodes[nodeIndex].particleIndices = indices;
        return nodeIndex;
    }

    const float childHalfSize = halfSize * 0.5f;
    for (int child = 0; child < 8; ++child) {
        if (buckets[child].empty())
            continue;
        const Vector3 childCenter(center.x + ((child & 1) != 0 ? childHalfSize : -childHalfSize),
                                  center.y + ((child & 2) != 0 ? childHalfSize : -childHalfSize),
                                  center.z + ((child & 4) != 0 ? childHalfSize : -childHalfSize));
        _nodes[nodeIndex].children[child] =
            buildNodeRecursive(particles, buckets[child], childCenter, childHalfSize, depth + 1);
        _nodes[nodeIndex].childMask |= static_cast<unsigned char>(1u << child);
    }
    return nodeIndex;
}

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

int Octree::childIndexForPosition(const Vector3& position, const Vector3& center)
{
    int index = 0;
    if (position.x >= center.x)
        index |= 1;
    if (position.y >= center.y)
        index |= 2;
    if (position.z >= center.z)
        index |= 4;
    return index;
}

bool Octree::hasChildren(const Node& node)
{
    return node.childMask != 0u;
}
