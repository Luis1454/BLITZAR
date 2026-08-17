/*
 * @file engine/physics/octree/src/Octree.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief CPU octree lifecycle and GPU export facade.
 */

#include "Octree.hpp"

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
