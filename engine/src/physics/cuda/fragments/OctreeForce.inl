// File: engine/src/physics/cuda/fragments/OctreeForce.inl
// Purpose: Engine implementation for the BLITZAR simulation core.

/*
 * Module: physics/cuda
 * Responsibility: Implement CPU-side octree force evaluation helpers.
 */

Vector3 Octree::computeForceRecursive(
    const std::vector<Particle> &particles,
    int nodeIndex,
    const Particle &particle,
    std::size_t selfIndex,
    const ForceLawPolicy &policy,
    OctreeOpeningCriterion criterion
) const {
    if (nodeIndex < 0) return Vector3(0.0f, 0.0f, 0.0f);
    const Node &node = _nodes[nodeIndex];
    if (node.mass <= 0.0f) return Vector3(0.0f, 0.0f, 0.0f);

    const Vector3 particlePos = particle.getPosition();
    if (!hasChildren(node)) {
        Vector3 force(0.0f, 0.0f, 0.0f);
        for (size_t i = 0; i < node.particleIndices.size(); ++i) {
            const int otherIndex = node.particleIndices[i];
            if (otherIndex == static_cast<int>(selfIndex)) continue;
            const Particle &other = particles[otherIndex];
            force += gravityAccelerationFromSource(particlePos, other.getPosition(), other.getMass(), policy);
        }
        return force;
    }

    const float size = node.halfSize * 2.0f;
    const bool containsSelf = std::fabs(particlePos.x - node.center.x) <= node.halfSize
        && std::fabs(particlePos.y - node.center.y) <= node.halfSize
        && std::fabs(particlePos.z - node.center.z) <= node.halfSize;
    const Vector3 direction = node.centerOfMass - particlePos;
    const float distance2 = softenedDistanceSquared(direction, policy);
    float criterionDistance = sqrtf(distance2);
    if (criterion == OctreeOpeningCriterion::Bounds) {
        const float dx = std::max(std::fabs(particlePos.x - node.center.x) - node.halfSize, 0.0f);
        const float dy = std::max(std::fabs(particlePos.y - node.center.y) - node.halfSize, 0.0f);
        const float dz = std::max(std::fabs(particlePos.z - node.center.z) - node.halfSize, 0.0f);
        criterionDistance = std::max(std::sqrt(dx * dx + dy * dy + dz * dz), 1.0e-6f);
    }
    if (!containsSelf && (size / criterionDistance) < policy.theta) {
        return gravityAccelerationFromSource(particlePos, node.centerOfMass, node.mass, policy);
    }

    Vector3 force(0.0f, 0.0f, 0.0f);
    for (int child = 0; child < 8; ++child) {
        if ((node.childMask & (1u << child)) == 0) continue;
        force += computeForceRecursive(particles, node.children[child], particle, selfIndex, policy, criterion);
    }
    return force;
}

Vector3 Octree::computeForceOn(const Particle &particle, std::size_t selfIndex, const ForceLawPolicy &policy, OctreeOpeningCriterion criterion) const
{
    if (_root < 0 || !_particlesRef.has_value()) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    return computeForceRecursive(_particlesRef->get(), _root, particle, selfIndex, policy, criterion);
}
