// File: engine/src/physics/cuda/fragments/OctreeBuild.inl
// Purpose: Engine implementation for the BLITZAR simulation core.

/*
 * Module: physics/cuda
 * Responsibility: Implement CPU-side octree construction primitives.
 */

/// Description: Executes the Node operation.
Octree::Node::Node()
    : center(0.0f, 0.0f, 0.0f),
      halfSize(0.0f),
      mass(0.0f),
      centerOfMass(0.0f, 0.0f, 0.0f),
      children{},
      childMask(0),
      particleIndices()
{
    children.fill(-1);
}

/// Description: Executes the Octree operation.
Octree::Octree() : _nodes(), _particlesRef(std::nullopt), _root(-1) {}
/// Description: Describes the destroy  octree operation contract.
Octree::~Octree() = default;

/// Description: Executes the clear operation.
void Octree::clear()
{
    _nodes.clear();
    _particlesRef.reset();
    _root = -1;
}

/// Description: Executes the getNodeCount operation.
std::size_t Octree::getNodeCount() const { return _nodes.size(); }
/// Description: Executes the getRootIndex operation.
int Octree::getRootIndex() const { return _root; }

/// Description: Executes the exportGpu operation.
void Octree::exportGpu(std::vector<GpuOctreeNode> &outNodes, std::vector<int> &outLeafIndices) const
{
    outNodes.clear();
    outLeafIndices.clear();
    outNodes.resize(_nodes.size());
    std::size_t totalLeafIndices = 0;
    for (const Node &node : _nodes) totalLeafIndices += node.particleIndices.size();
    outLeafIndices.reserve(totalLeafIndices);

    for (size_t i = 0; i < _nodes.size(); ++i) {
        const Node &src = _nodes[i];
        GpuOctreeNode dst{};
        dst.centerX = src.center.x;
        dst.centerY = src.center.y;
        dst.centerZ = src.center.z;
        dst.halfSize = src.halfSize;
        dst.mass = src.mass;
        dst.comX = src.centerOfMass.x;
        dst.comY = src.centerOfMass.y;
        dst.comZ = src.centerOfMass.z;
        for (int c = 0; c < 8; ++c) dst.children[c] = src.children[c];
        dst.childMask = src.childMask;
        dst.leafStart = static_cast<int>(outLeafIndices.size());
        dst.leafCount = static_cast<int>(src.particleIndices.size());
        dst.parentIndex = -1;
        dst.nextIndex = -1;
        for (int leafIndex : src.particleIndices) outLeafIndices.push_back(leafIndex);
        outNodes[i] = dst;
    }
}

/// Description: Executes the childIndexForPosition operation.
int Octree::childIndexForPosition(const Vector3 &position, const Vector3 &center)
{
    int child = 0;
    if (position.x >= center.x) child |= 1;
    if (position.y >= center.y) child |= 2;
    if (position.z >= center.z) child |= 4;
    return child;
}

/// Description: Executes the hasChildren operation.
bool Octree::hasChildren(const Node &node) { return node.childMask != 0; }

/// Description: Describes the build node recursive operation contract.
int Octree::buildNodeRecursive(
    const std::vector<Particle> &particles,
    const std::vector<int> &indices,
    const Vector3 &center,
    float halfSize,
    int depth
) {
    Node node;
    node.center = center;
    node.halfSize = halfSize;

    float totalMass = 0.0f;
    Vector3 weightedCenter(0.0f, 0.0f, 0.0f);
    for (size_t i = 0; i < indices.size(); ++i) {
        const Particle &p = particles[indices[i]];
        const float mass = p.getMass();
        totalMass += mass;
        weightedCenter += p.getPosition() * mass;
    }
    node.mass = totalMass;
    node.centerOfMass = totalMass > 0.0f ? (weightedCenter / totalMass) : center;

    const int nodeIndex = static_cast<int>(_nodes.size());
    _nodes.push_back(node);
    if (indices.size() <= kOctreeLeafCapacity || halfSize < 0.01f || depth > kOctreeMaxDepth) {
        auto &leafIndices = _nodes[nodeIndex].particleIndices;
        leafIndices.resize(indices.size());
        for (size_t i = 0; i < indices.size(); ++i) leafIndices[i] = indices[i];
        return nodeIndex;
    }

    std::array<std::vector<int>, 8> buckets;
    for (int i = 0; i < 8; ++i) buckets[i].reserve(indices.size() / 4 + 1);
    int nonEmptyBuckets = 0;
    for (size_t i = 0; i < indices.size(); ++i) {
        const int particleIndex = indices[i];
        const int child = childIndexForPosition(particles[particleIndex].getPosition(), center);
        if (buckets[child].empty()) ++nonEmptyBuckets;
        buckets[child].push_back(particleIndex);
    }

    if (nonEmptyBuckets <= 1) {
        auto &leafIndices = _nodes[nodeIndex].particleIndices;
        leafIndices.resize(indices.size());
        for (size_t i = 0; i < indices.size(); ++i) leafIndices[i] = indices[i];
        return nodeIndex;
    }

    const float childHalf = halfSize * 0.5f;
    for (int child = 0; child < 8; ++child) {
        if (buckets[child].empty()) continue;
        const Vector3 childCenter(
            center.x + ((child & 1) ? childHalf : -childHalf),
            center.y + ((child & 2) ? childHalf : -childHalf),
            center.z + ((child & 4) ? childHalf : -childHalf)
        );
        _nodes[nodeIndex].children[child] = buildNodeRecursive(particles, buckets[child], childCenter, childHalf, depth + 1);
        _nodes[nodeIndex].childMask |= static_cast<unsigned char>(1u << child);
    }
    return nodeIndex;
}

/// Description: Executes the build operation.
void Octree::build(const std::vector<Particle> &particles)
{
    clear();
    _particlesRef = std::cref(particles);
    if (particles.empty()) return;
    _nodes.reserve(particles.size() * 2);

    Vector3 minPos = particles[0].getPosition();
    Vector3 maxPos = particles[0].getPosition();
    for (size_t i = 1; i < particles.size(); ++i) {
        const Vector3 pos = particles[i].getPosition();
        minPos.x = std::min(minPos.x, pos.x);
        minPos.y = std::min(minPos.y, pos.y);
        minPos.z = std::min(minPos.z, pos.z);
        maxPos.x = std::max(maxPos.x, pos.x);
        maxPos.y = std::max(maxPos.y, pos.y);
        maxPos.z = std::max(maxPos.z, pos.z);
    }

    const Vector3 center((minPos.x + maxPos.x) * 0.5f, (minPos.y + maxPos.y) * 0.5f, (minPos.z + maxPos.z) * 0.5f);
    const float sizeX = maxPos.x - minPos.x;
    const float sizeY = maxPos.y - minPos.y;
    const float sizeZ = maxPos.z - minPos.z;
    const float halfSize = std::max(0.5f * std::max(sizeX, std::max(sizeY, sizeZ)), 0.01f) + 0.001f;

    std::vector<int> rootIndices(particles.size());
    std::iota(rootIndices.begin(), rootIndices.end(), 0);
    _root = buildNodeRecursive(particles, rootIndices, center, halfSize, 0);
}
