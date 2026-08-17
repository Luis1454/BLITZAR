/*
 * @file engine/src/physics/octree/OctreeBuild.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief CPU octree construction.
 */

#include "physics/octree/Octree.hpp"
#include <algorithm>
#include <cstddef>
#include <numeric>

namespace blitzar_physics_octree {
constexpr std::size_t kLeafCapacity = 16u;
constexpr int kMaxDepth = 24;
constexpr float kMinHalfSize = 0.01f;
} // namespace blitzar_physics_octree

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
