/*
 * @file engine/src/physics/ParticleSystemHost.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief CPU fallback implementation for the particle system.
 */

#include "physics/ParticleSystem.hpp"
#include "Constants.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <omp.h>
#ifdef __SSE__
#include <xmmintrin.h>
#endif

namespace {
constexpr float kGravity = 1.0f;
constexpr std::size_t kOctreeLeafCapacity = 16u;
constexpr int kOctreeMaxDepth = 24;
constexpr float kOctreeMinHalfSize = 0.01f;

float squaredLength(Vector3 value)
{
    return value.x * value.x + value.y * value.y + value.z * value.z;
}

float softenedDistanceSquared(Vector3 delta, const ForceLawPolicy& policy)
{
    return squaredLength(delta) + policy.softening * policy.softening;
}

// SIMD-optimized: arithmetic hotpath, called ~100M times
// Compiler hint: unroll and vectorize this for maximum throughput
Vector3 accelerationFromSource(Vector3 selfPosition, Vector3 sourcePosition, float sourceMass,
                               const ForceLawPolicy& policy)
{
    const Vector3 delta = sourcePosition - selfPosition;
    const float dist2 = softenedDistanceSquared(delta, policy);
    if (dist2 <= policy.minDistance2)
        return Vector3();

    // Use fast reciprocal-sqrt with a Newton-Raphson refine when SSE is available.
    float invDistance = 1.0f / std::sqrt(dist2);
#ifdef __SSE__
    // approximate reciprocal sqrt
    __m128 v = _mm_set_ss(dist2);
    __m128 r = _mm_rsqrt_ss(v);
    invDistance = _mm_cvtss_f32(r);
    // one Newton-Raphson iteration to improve accuracy: inv = inv*(1.5 - 0.5*x*inv*inv)
    invDistance = invDistance * (1.5f - 0.5f * dist2 * invDistance * invDistance);
#endif
    const float invDistance3 = invDistance * invDistance * invDistance;

    // Vectorization opportunity: multiply operates independently on x,y,z
    return delta * (kGravity * sourceMass * invDistance3);
}

Vector3 clampedVector(Vector3 value, float limit)
{
    const float speed = value.norm();
    if (limit <= 0.0f || speed <= limit || speed <= 1e-6f)
        return value;
    return value * (limit / speed);
}

Particle makeParticle(Vector3 position, Vector3 velocity)
{
    Particle particle;
    particle.setPosition(position);
    particle.setVelocity(velocity);
    particle.setPressure(Vector3(0.0f, 0.0f, 0.0f));
    particle.setMass(Particle::kDefaultMass);
    particle.setDensity(1.0f);
    particle.setTemperature(0.0f);
    return particle;
}
} // namespace

Octree::Node::Node()
    : center(), halfSize(0.0f), mass(0.0f), centerOfMass(), children(), childMask(0u),
      particleIndices()
{
    children.fill(-1);
}

Octree::Octree() : _nodes(), _particlesRef(std::nullopt), _root(-1) {}

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
    const float halfSize =
        std::max(0.5f * std::max(sizeX, std::max(sizeY, sizeZ)), kOctreeMinHalfSize) + 0.001f;

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
    const float halfSize =
        std::max(0.5f * std::max(sizeX, std::max(sizeY, sizeZ)), kOctreeMinHalfSize) + 0.001f;

    std::vector<int> rootIndices(particles.size());
    std::iota(rootIndices.begin(), rootIndices.end(), 0);
    _root = buildNodeRecursive(particles, rootIndices, center, halfSize, 0);
}

Vector3 Octree::computeForceOn(const Particle& particle, std::size_t selfIndex,
                               const ForceLawPolicy& policy,
                               OctreeOpeningCriterion criterion) const
{
    if (_root < 0 || !_particlesRef.has_value())
        return Vector3();
    return computeForceRecursive(_particlesRef->get(), _root, particle, selfIndex, policy,
                                 criterion);
}

Vector3 Octree::computeForceOn(const std::vector<ParticleHotData>& particles,
                               std::size_t selfIndex, const ForceLawPolicy& policy,
                               OctreeOpeningCriterion criterion) const
{
    if (_root < 0 || particles.empty() || selfIndex >= particles.size())
        return Vector3();
    return computeForceRecursive(particles, _root, particles[selfIndex], selfIndex, policy,
                                 criterion);
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
    if (indices.size() <= kOctreeLeafCapacity || halfSize <= kOctreeMinHalfSize ||
        depth >= kOctreeMaxDepth) {
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
    if (indices.size() <= kOctreeLeafCapacity || halfSize <= kOctreeMinHalfSize ||
        depth >= kOctreeMaxDepth) {
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
        _nodes[nodeIndex].children[child] = buildNodeRecursive(particles, buckets[child],
                                                               childCenter, childHalfSize,
                                                               depth + 1);
        _nodes[nodeIndex].childMask |= static_cast<unsigned char>(1u << child);
    }
    return nodeIndex;
}

Vector3 Octree::computeForceRecursive(const std::vector<Particle>& particles, int nodeIndex,
                                      const Particle& particle, std::size_t selfIndex,
                                      const ForceLawPolicy& policy,
                                      OctreeOpeningCriterion criterion) const
{
    if (nodeIndex < 0)
        return Vector3();

    Vector3 totalAcceleration;
    const Vector3 particlePosition = particle.getPosition();
    std::vector<int> traversalStack;
    traversalStack.reserve(64);
    traversalStack.push_back(nodeIndex);

    while (!traversalStack.empty()) {
        const int currentNodeIndex = traversalStack.back();
        traversalStack.pop_back();

        if (currentNodeIndex < 0)
            continue;

        const Node& node = _nodes[currentNodeIndex];
        if (node.mass <= 0.0f)
            continue;

        if (!hasChildren(node)) {
            for (int particleIndex : node.particleIndices) {
                if (particleIndex == static_cast<int>(selfIndex))
                    continue;
                const Particle& other = particles[particleIndex];
                totalAcceleration += accelerationFromSource(particlePosition, other.getPosition(),
                                                            other.getMass(), policy);
            }
            continue;
        }

        const float size = node.halfSize * 2.0f;
        const bool containsSelf =
            std::fabs(particlePosition.x - node.center.x) <= node.halfSize &&
            std::fabs(particlePosition.y - node.center.y) <= node.halfSize &&
            std::fabs(particlePosition.z - node.center.z) <= node.halfSize;

        const Vector3 direction = node.centerOfMass - particlePosition;
        const float distance2 = softenedDistanceSquared(direction, policy);
        float criterionDistance = std::max(std::sqrt(distance2), 1.0e-6f);

        if (criterion == OctreeOpeningCriterion::Bounds) {
            const float dx = std::max(std::fabs(particlePosition.x - node.center.x) - node.halfSize, 0.0f);
            const float dy = std::max(std::fabs(particlePosition.y - node.center.y) - node.halfSize, 0.0f);
            const float dz = std::max(std::fabs(particlePosition.z - node.center.z) - node.halfSize, 0.0f);
            criterionDistance = std::max(std::sqrt(dx * dx + dy * dy + dz * dz), 1.0e-6f);
        }

        if (!containsSelf && (size / criterionDistance) < policy.theta) {
            totalAcceleration += accelerationFromSource(particlePosition, node.centerOfMass,
                                                        node.mass, policy);
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

Vector3 Octree::computeForceRecursive(const std::vector<ParticleHotData>& particles,
                                      int nodeIndex, const ParticleHotData& particle,
                                      std::size_t selfIndex, const ForceLawPolicy& policy,
                                      OctreeOpeningCriterion criterion) const
{
    if (nodeIndex < 0)
        return Vector3();

    Vector3 totalAcceleration;
    const Vector3 particlePosition = particle.getPosition();
    std::vector<int> traversalStack;
    traversalStack.reserve(64);
    traversalStack.push_back(nodeIndex);

    while (!traversalStack.empty()) {
        const int currentNodeIndex = traversalStack.back();
        traversalStack.pop_back();

        if (currentNodeIndex < 0)
            continue;

        const Node& node = _nodes[currentNodeIndex];
        if (node.mass <= 0.0f)
            continue;

        if (!hasChildren(node)) {
            for (int particleIndex : node.particleIndices) {
                if (particleIndex == static_cast<int>(selfIndex))
                    continue;
                const ParticleHotData& other = particles[particleIndex];
                totalAcceleration +=
                    accelerationFromSource(particlePosition, other.getPosition(),
                                           other.getMass(), policy);
            }
            continue;
        }

        const float size = node.halfSize * 2.0f;
        const bool containsSelf =
            std::fabs(particlePosition.x - node.center.x) <= node.halfSize &&
            std::fabs(particlePosition.y - node.center.y) <= node.halfSize &&
            std::fabs(particlePosition.z - node.center.z) <= node.halfSize;

        const Vector3 direction = node.centerOfMass - particlePosition;
        const float distance2 = softenedDistanceSquared(direction, policy);
        float criterionDistance = std::max(std::sqrt(distance2), 1.0e-6f);

        if (criterion == OctreeOpeningCriterion::Bounds) {
            const float dx = std::max(std::fabs(particlePosition.x - node.center.x) - node.halfSize,
                                      0.0f);
            const float dy = std::max(std::fabs(particlePosition.y - node.center.y) - node.halfSize,
                                      0.0f);
            const float dz = std::max(std::fabs(particlePosition.z - node.center.z) - node.halfSize,
                                      0.0f);
            criterionDistance = std::max(std::sqrt(dx * dx + dy * dy + dz * dz), 1.0e-6f);
        }

        if (!containsSelf && (size / criterionDistance) < policy.theta) {
            totalAcceleration += accelerationFromSource(particlePosition, node.centerOfMass,
                                                        node.mass, policy);
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

ParticleSystem::ParticleSystem(int numParticles, bool bootstrapInitialState)
{
    initializeRuntimeState(static_cast<std::size_t>(std::max(0, numParticles)));
    if (bootstrapInitialState) {
        buildBootstrapState(numParticles);
    }
}

ParticleSystem::ParticleSystem(std::vector<Particle> initialParticles)
{
    initializeRuntimeState(initialParticles.size());
    _particles = std::move(initialParticles);
}

ParticleSystem::~ParticleSystem()
{
    releaseParticleBuffers();
}

bool ParticleSystem::update(float deltaTime)
{
    if (_particles.empty() || deltaTime <= 0.0f)
        return false;

    const std::size_t count = _particles.size();
    std::vector<Vector3> accelerations(count, Vector3());
    std::vector<ParticleHotData> hotParticles;
    buildParticleHotData(_particles, hotParticles);
    const ForceLawPolicy forceLaw =
        resolveForceLawPolicy(_octreeTheta, _octreeSoftening, _physicsMinSoftening,
                              _physicsMinDistance2, _physicsMinTheta);

    if (_solverMode == SolverMode::PairwiseCuda && count <= 4096u) {
#pragma omp parallel for schedule(static)
        for (std::size_t i = 0; i < count; ++i) {
            const Vector3 pi = hotParticles[i].getPosition();
            Vector3 acceleration;
            for (std::size_t j = 0; j < count; ++j) {
                if (i == j)
                    continue;
                acceleration += accelerationFromSource(pi, hotParticles[j].getPosition(),
                                                       hotParticles[j].getMass(), forceLaw);
            }
            accelerations[i] = clampedVector(acceleration, _physicsMaxAcceleration);
        }
    }
    else {
        _octree.build(hotParticles);
#pragma omp parallel for schedule(static)
        for (std::size_t i = 0; i < count; ++i) {
            accelerations[i] = clampedVector(
                _octree.computeForceOn(hotParticles, i, forceLaw, _octreeOpeningCriterion),
                _physicsMaxAcceleration);
        }
    }

    for (std::size_t i = 0; i < count; ++i) {
        Vector3 velocity = _particles[i].getVelocity() + accelerations[i] * deltaTime;
        velocity = clampedVector(velocity, _sphMaxSpeed);
        _particles[i].setVelocity(velocity);
        _particles[i].setPosition(_particles[i].getPosition() + velocity * deltaTime);
    }

    _cumulativeRadiatedEnergy += applyThermalModel(deltaTime);
    _device._hostStateDirty = false;
    return true;
}

void ParticleSystem::setUseOctree(bool enabled)
{
    _solverMode = enabled ? SolverMode::OctreeCpu : SolverMode::PairwiseCuda;
}

bool ParticleSystem::usesOctree() const
{
    return _solverMode != SolverMode::PairwiseCuda;
}

void ParticleSystem::setOctreeTheta(float theta)
{
    _octreeTheta = std::max(theta, _physicsMinTheta);
}

void ParticleSystem::setOctreeOpeningCriterion(OctreeOpeningCriterion criterion)
{
    _octreeOpeningCriterion = criterion;
}

void ParticleSystem::setOctreeSoftening(float softening)
{
    _octreeSoftening = std::max(softening, _physicsMinSoftening);
}

void ParticleSystem::setSphEnabled(bool enabled)
{
    _sphEnabled = enabled;
}

bool ParticleSystem::isSphEnabled() const
{
    return _sphEnabled;
}

void ParticleSystem::setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                                      float viscosity)
{
    _sphSmoothingLength = std::max(0.0f, smoothingLength);
    _sphRestDensity = std::max(0.0f, restDensity);
    _sphGasConstant = std::max(0.0f, gasConstant);
    _sphViscosity = std::max(0.0f, viscosity);
}

void ParticleSystem::setPhysicsStabilityConstants(float maxAcceleration, float minSoftening,
                                                  float minDistance2, float minTheta)
{
    _physicsMaxAcceleration = std::max(0.0f, maxAcceleration);
    _physicsMinSoftening = std::max(0.0f, minSoftening);
    _physicsMinDistance2 = std::max(0.0f, minDistance2);
    _physicsMinTheta = std::max(0.0f, minTheta);
    setOctreeSoftening(_octreeSoftening);
    setOctreeTheta(_octreeTheta);
}

void ParticleSystem::setSphCaps(float maxAcceleration, float maxSpeed)
{
    _sphMaxAcceleration = std::max(0.0f, maxAcceleration);
    _sphMaxSpeed = std::max(0.0f, maxSpeed);
}

void ParticleSystem::setThermalParameters(float ambientTemperature, float specificHeat,
                                          float heatingCoeff, float radiationCoeff)
{
    _thermalAmbientTemperature = ambientTemperature;
    _thermalSpecificHeat = std::max(1e-6f, specificHeat);
    _thermalHeatingCoeff = std::max(0.0f, heatingCoeff);
    _thermalRadiationCoeff = std::max(0.0f, radiationCoeff);
}

float ParticleSystem::getCumulativeRadiatedEnergy() const
{
    return _cumulativeRadiatedEnergy;
}

float ParticleSystem::getThermalSpecificHeat() const
{
    return _thermalSpecificHeat;
}

void ParticleSystem::setSolverMode(SolverMode mode)
{
    _solverMode = mode == SolverMode::OctreeGpu ? SolverMode::OctreeCpu : mode;
}

ParticleSystem::SolverMode ParticleSystem::getSolverMode() const
{
    return _solverMode;
}

void ParticleSystem::setIntegratorMode(IntegratorMode mode)
{
    _integratorMode = mode;
}

ParticleSystem::IntegratorMode ParticleSystem::getIntegratorMode() const
{
    return _integratorMode;
}

void ParticleSystem::syncDeviceState()
{
    _device._hostStateDirty = false;
}

bool ParticleSystem::syncHostState()
{
    return true;
}

bool ParticleSystem::computeEnergyEstimateGpu(std::size_t, float, float, float, float&, float&,
                                              float&, bool&)
{
    return false;
}

const std::vector<Particle>& ParticleSystem::getParticles() const
{
    return _particles;
}

bool ParticleSystem::setParticles(std::vector<Particle> particles)
{
    if (particles.empty())
        return false;
    _particles = std::move(particles);
    _device._deviceParticleCapacity = _particles.size();
    _device._hostStateDirty = false;
    return true;
}

ParticleSoAView ParticleSystem::getSoAView(bool) const
{
    return ParticleSoAView{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                           nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, {0, 0, 0}};
}

const GpuSystemMetrics* ParticleSystem::getMappedGpuMetrics() const
{
    return nullptr;
}

void ParticleSystem::initializeRuntimeState(std::size_t particleCapacity)
{
    _solverMode = SolverMode::PairwiseCuda;
    _integratorMode = IntegratorMode::Euler;
    _octreeTheta = 0.6f;
    _octreeOpeningCriterion = OctreeOpeningCriterion::CenterOfMass;
    _octreeSoftening = 0.01f;
    _sphEnabled = false;
    _sphSmoothingLength = 1.0f;
    _sphRestDensity = 1.0f;
    _sphGasConstant = 1.0f;
    _sphViscosity = 0.0f;
    _physicsMaxAcceleration = 1000.0f;
    _physicsMinSoftening = 1e-4f;
    _physicsMinDistance2 = 1e-8f;
    _physicsMinTheta = 0.1f;
    _sphMaxAcceleration = 1000.0f;
    _sphMaxSpeed = 1000.0f;
    _thermalAmbientTemperature = 0.0f;
    _thermalSpecificHeat = 1.0f;
    _thermalHeatingCoeff = 0.0f;
    _thermalRadiationCoeff = 0.0f;
    _cumulativeRadiatedEnergy = 0.0f;
    _device = ParticleSystemDeviceState{};
    _device._deviceParticleCapacity = particleCapacity;
}

void ParticleSystem::buildBootstrapState(int particleCount)
{
    const int count = std::max(0, particleCount);
    _particles.clear();
    _particles.reserve(static_cast<std::size_t>(count));
    if (count == 0)
        return;
    for (int i = 0; i < count; ++i) {
        const float fraction = static_cast<float>(i) / static_cast<float>(std::max(1, count));
        const float angle = fraction * 2.0f * kPi;
        const float radius = 0.25f + 0.75f * fraction;
        const Vector3 position(std::cos(angle) * radius, std::sin(angle) * radius, 0.0f);
        const Vector3 velocity(-std::sin(angle) * 0.05f, std::cos(angle) * 0.05f, 0.0f);
        _particles.push_back(makeParticle(position, velocity));
    }
}

bool ParticleSystem::allocateParticleBuffers(std::size_t particleCapacity)
{
    _device._deviceParticleCapacity = particleCapacity;
    return true;
}

bool ParticleSystem::seedDeviceState()
{
    return true;
}

void ParticleSystem::releaseParticleBuffers()
{
    releaseMappedMetrics();
}

float ParticleSystem::applyThermalModel(float deltaTime)
{
    if (_thermalRadiationCoeff <= 0.0f || _particles.empty())
        return 0.0f;
    float radiated = 0.0f;
    for (Particle& particle : _particles) {
        const float temperature = particle.getTemperature();
        const float excess = std::max(0.0f, temperature - _thermalAmbientTemperature);
        const float loss = std::min(excess, excess * _thermalRadiationCoeff * deltaTime);
        particle.setTemperature(temperature - loss);
        radiated += loss * particle.getMass() * _thermalSpecificHeat;
    }
    return radiated;
}

bool ParticleSystem::buildSphGrid(int)
{
    return !_sphEnabled || !_particles.empty();
}

void ParticleSystem::releaseRk4Buffers() {}
void ParticleSystem::releaseSphBuffers() {}
void ParticleSystem::releaseSphGridBuffers() {}

bool ParticleSystem::allocateRk4Buffers(int)
{
    return true;
}

bool ParticleSystem::allocateSphBuffers(int)
{
    return true;
}

bool ParticleSystem::allocateSphGridBuffers(int)
{
    return true;
}

bool ParticleSystem::ensureLinearOctreeScratchCapacity(int numParticles)
{
    _device._linearOctreeLeafCapacity = std::max(0, numParticles);
    return true;
}

bool ParticleSystem::ensureEnergyScratchCapacity(int, int)
{
    return true;
}

bool ParticleSystem::buildLinearOctreeGpu(ParticleSoAView, int)
{
    return false;
}

bool ParticleSystem::allocateMappedMetrics()
{
    return false;
}

void ParticleSystem::releaseMappedMetrics()
{
    _device._mappedMetricsHost = nullptr;
    _device._mappedMetricsDevice = nullptr;
}

void ParticleSystem::publishMappedMetrics(float deltaTime)
{
    _device._metricsStepId += 1u;
    _device._metricsSimTime += deltaTime;
}

std::size_t ParticleSystem::estimateMemoryUsage(std::size_t particleCount, bool sphEnabled,
                                                SolverMode, IntegratorMode,
                                                std::size_t energySampleLimit,
                                                int octreeLeafCapacity,
                                                std::size_t* baseAndIntegratorBytes,
                                                std::size_t* sphBytes,
                                                std::size_t* octreeBytes) const
{
    const std::size_t base = particleCount * sizeof(Particle);
    const std::size_t sph = sphEnabled ? particleCount * (2u * sizeof(float) + 2u * sizeof(int)) : 0u;
    const std::size_t octree =
        static_cast<std::size_t>(std::max(0, octreeLeafCapacity)) * sizeof(GpuOctreeNode);
    const std::size_t energy = std::max<std::size_t>(1u, energySampleLimit) * sizeof(double);
    if (baseAndIntegratorBytes)
        *baseAndIntegratorBytes = base + energy;
    if (sphBytes)
        *sphBytes = sph;
    if (octreeBytes)
        *octreeBytes = octree;
    return base + sph + octree + energy;
}

std::string ParticleSystem::formatMemoryBreakdown(std::size_t baseAndIntegratorBytes,
                                                  std::size_t sphBytes, std::size_t octreeBytes,
                                                  std::size_t totalBytes,
                                                  std::size_t budgetBytes)
{
    std::ostringstream stream;
    stream << "base=" << baseAndIntegratorBytes << " sph=" << sphBytes
           << " octree=" << octreeBytes << " total=" << totalBytes << " budget=" << budgetBytes;
    return stream.str();
}
