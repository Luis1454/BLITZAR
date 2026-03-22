#include "ui/OctreeOverlay.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <vector>

namespace grav_qt {

struct OverlayBounds final {
    float centerX;
    float centerY;
    float centerZ;
    float halfSize;
};

static constexpr std::size_t kMaxOverlayNodes = 4096u;

static OverlayBounds computeBounds(const std::vector<RenderParticle> &particles)
{
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const RenderParticle &particle : particles) {
        minX = std::min(minX, particle.x);
        minY = std::min(minY, particle.y);
        minZ = std::min(minZ, particle.z);
        maxX = std::max(maxX, particle.x);
        maxY = std::max(maxY, particle.y);
        maxZ = std::max(maxZ, particle.z);
    }
    const float maxExtent = std::max({maxX - minX, maxY - minY, maxZ - minZ, 1.0e-3f});
    return OverlayBounds{
        (minX + maxX) * 0.5f,
        (minY + maxY) * 0.5f,
        (minZ + maxZ) * 0.5f,
        std::max(maxExtent * 0.5f, 1.0e-3f) * 1.001f
    };
}

static int childIndexForParticle(const RenderParticle &particle, float centerX, float centerY, float centerZ)
{
    int childIndex = 0;
    if (particle.x >= centerX) {
        childIndex |= 1;
    }
    if (particle.y >= centerY) {
        childIndex |= 2;
    }
    if (particle.z >= centerZ) {
        childIndex |= 4;
    }
    return childIndex;
}

static void appendNodes(const std::vector<RenderParticle> &particles, const std::vector<std::size_t> &indices, const OctreeOverlayNode &node, int maxDepth, std::vector<OctreeOverlayNode> &nodes)
{
    if (indices.empty() || nodes.size() >= kMaxOverlayNodes) {
        return;
    }
    nodes.push_back(node);
    if (node.depth >= maxDepth || indices.size() <= 1u) {
        return;
    }

    const float childHalfSize = node.halfSize * 0.5f;
    if (childHalfSize <= 1.0e-6f) {
        return;
    }

    std::array<std::vector<std::size_t>, 8> children{};
    for (const std::size_t index : indices) {
        children[static_cast<std::size_t>(childIndexForParticle(particles[index], node.centerX, node.centerY, node.centerZ))].push_back(index);
    }
    for (int child = 0; child < 8; ++child) {
        const std::vector<std::size_t> &childIndices = children[static_cast<std::size_t>(child)];
        if (childIndices.empty()) {
            continue;
        }
        appendNodes(
            particles,
            childIndices,
            OctreeOverlayNode{
                node.centerX + ((child & 1) != 0 ? childHalfSize : -childHalfSize),
                node.centerY + ((child & 2) != 0 ? childHalfSize : -childHalfSize),
                node.centerZ + ((child & 4) != 0 ? childHalfSize : -childHalfSize),
                childHalfSize,
                node.depth + 1
            },
            maxDepth,
            nodes);
        if (nodes.size() >= kMaxOverlayNodes) {
            return;
        }
    }
}

std::vector<OctreeOverlayNode> OctreeOverlay::build(const std::vector<RenderParticle> &particles, int maxDepth)
{
    std::vector<OctreeOverlayNode> nodes;
    if (particles.empty() || maxDepth < 0) {
        return nodes;
    }

    std::vector<std::size_t> indices;
    indices.reserve(particles.size());
    for (std::size_t index = 0; index < particles.size(); ++index) {
        indices.push_back(index);
    }
    const OverlayBounds bounds = computeBounds(particles);
    appendNodes(particles, indices, OctreeOverlayNode{bounds.centerX, bounds.centerY, bounds.centerZ, bounds.halfSize, 0}, maxDepth, nodes);
    return nodes;
}

} // namespace grav_qt
