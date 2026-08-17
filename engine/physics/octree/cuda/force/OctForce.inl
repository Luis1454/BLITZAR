/*
 * @file engine/physics/octree/cuda/force/OctForce.inl
 * @author Luis1454
 * @project BLITZAR
 * @brief Physics and CUDA implementation for the deterministic simulation core.
 */

/*
 * Module: cuda
 * Responsibility: Implement CPU-side octree force evaluation helpers.
 */

/*
 * @brief Documents the compute force recursive operation contract.
 * @param particles Input value used by this contract.
 * @param nodeIndex Input value used by this contract.
 * @param particle Input value used by this contract.
 * @param selfIndex Input value used by this contract.
 * @param policy Input value used by this contract.
 * @param criterion Input value used by this contract.
 * @return Vector3 Octree:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
Vector3 Octree::computeForceRecursive(const std::vector<Particle>& particles, int nodeIndex,
                                      const Particle& particle, std::size_t selfIndex,
                                      const ForceLawPolicy& policy,
                                      OctreeOpeningCriterion criterion,
                                      float cutoffSquared) const
{
    if (nodeIndex < 0)
        return Vector3(0.0f, 0.0f, 0.0f);
    const Node& node = _nodes[nodeIndex];
    if (node.mass <= 0.0f)
        return Vector3(0.0f, 0.0f, 0.0f);

    const Vector3 particlePos = particle.getPosition();
    if (cutoffSquared > 0.0f) {
        const float dx = max(fabsf(particlePos.x - node.center.x) - node.halfSize, 0.0f);
        const float dy = max(fabsf(particlePos.y - node.center.y) - node.halfSize, 0.0f);
        const float dz = max(fabsf(particlePos.z - node.center.z) - node.halfSize, 0.0f);
        if (dx * dx + dy * dy + dz * dz > cutoffSquared)
            return Vector3(0.0f, 0.0f, 0.0f);
    }
    if (!hasChildren(node)) {
        Vector3 force(0.0f, 0.0f, 0.0f);
        for (size_t i = 0; i < node.particleIndices.size(); ++i) {
            const int otherIndex = node.particleIndices[i];
            if (otherIndex == static_cast<int>(selfIndex))
                continue;
            const Particle& other = particles[otherIndex];
            if (cutoffSquared > 0.0f) {
                const Vector3 delta = other.getPosition() - particlePos;
                if (dot(delta, delta) > cutoffSquared)
                    continue;
            }
            force += blitzarAccelerationFromSource(particlePos, other.getPosition(),
                                                   other.getMass(), policy);
        }
        return force;
    }

    const float size = node.halfSize * 2.0f;
    const bool containsSelf = std::fabs(particlePos.x - node.center.x) <= node.halfSize &&
                              std::fabs(particlePos.y - node.center.y) <= node.halfSize &&
                              std::fabs(particlePos.z - node.center.z) <= node.halfSize;
    if (!containsSelf) {
        float criterionDistanceSquared =
            softenedDistanceSquared(node.centerOfMass - particlePos, policy);
        if (criterion == OctreeOpeningCriterion::Bounds) {
            const float dx =
                std::max(std::fabs(particlePos.x - node.center.x) - node.halfSize, 0.0f);
            const float dy =
                std::max(std::fabs(particlePos.y - node.center.y) - node.halfSize, 0.0f);
            const float dz =
                std::max(std::fabs(particlePos.z - node.center.z) - node.halfSize, 0.0f);
            criterionDistanceSquared = std::max(dx * dx + dy * dy + dz * dz, 1.0e-12f);
        }
        const float openingThreshold = policy.theta * policy.theta * criterionDistanceSquared;
        const float maxDx = fabsf(particlePos.x - node.center.x) + node.halfSize;
        const float maxDy = fabsf(particlePos.y - node.center.y) + node.halfSize;
        const float maxDz = fabsf(particlePos.z - node.center.z) + node.halfSize;
        const bool insideCutoff = cutoffSquared <= 0.0f ||
                                  maxDx * maxDx + maxDy * maxDy + maxDz * maxDz <= cutoffSquared;
        if (insideCutoff && size * size < openingThreshold) {
            return blitzarAccelerationFromSource(particlePos, node.centerOfMass, node.mass, policy);
        }
    }

    Vector3 force(0.0f, 0.0f, 0.0f);
    for (int child = 0; child < 8; ++child) {
        if ((node.childMask & (1u << child)) == 0)
            continue;
        force += computeForceRecursive(particles, node.children[child], particle, selfIndex, policy,
                                       criterion, cutoffSquared);
    }
    return force;
}

/*
 * @brief Documents the compute force on operation contract.
 * @param particle Input value used by this contract.
 * @param selfIndex Input value used by this contract.
 * @param policy Input value used by this contract.
 * @param criterion Input value used by this contract.
 * @return Vector3 Octree:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
Vector3 Octree::computeForceOn(const Particle& particle, std::size_t selfIndex,
                               const ForceLawPolicy& policy, OctreeOpeningCriterion criterion,
                               float cutoffSquared) const
{
    if (_root < 0 || !_particlesRef.has_value()) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }
    return computeForceRecursive(_particlesRef->get(), _root, particle, selfIndex, policy,
                                 criterion, cutoffSquared);
}
