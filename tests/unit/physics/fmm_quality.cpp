/*
 * @file tests/unit/physics/fmm_quality.cpp
 * @brief Adaptive FMM qualification on deterministic astrophysical topologies.
 */

#include "physics/fmm/FmmCpu.hpp"
#include "physics/core/ForceLawPolicy.hpp"
#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

static float randomUnit(std::uint32_t& state)
{
    state = state * 1664525u + 1013904223u;
    return static_cast<float>(state >> 8u) / 16777215.0f;
}

static Particle particle(Vector3 position)
{
    Particle result;
    result.setPosition(position);
    result.setMass(0.01f);
    return result;
}

static std::vector<Particle> plummerParticles(std::size_t count)
{
    std::uint32_t state = 712367u;
    std::vector<Particle> particles;
    particles.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const float radius =
            0.8f / std::sqrt(std::pow(std::max(randomUnit(state), 1.0e-4f), -2.0f / 3.0f) - 1.0f);
        const float z = 2.0f * randomUnit(state) - 1.0f;
        const float phi = 6.2831853f * randomUnit(state);
        const float radial = std::sqrt(std::max(0.0f, 1.0f - z * z));
        particles.push_back(particle(
            Vector3(radius * radial * std::cos(phi), radius * radial * std::sin(phi), radius * z)));
    }
    return particles;
}

static std::vector<Particle> collisionParticles(std::size_t count)
{
    std::uint32_t state = 918273u;
    std::vector<Particle> particles;
    particles.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const float side = index < count / 2u ? -1.0f : 1.0f;
        const auto noise = [&state]() {
            return (randomUnit(state) + randomUnit(state) - 1.0f) * 0.8f;
        };
        Particle value = particle(Vector3(side * 3.0f + noise(), noise(), noise()));
        value.setVelocity(Vector3(-side * 0.1f, 0.0f, 0.0f));
        particles.push_back(value);
    }
    return particles;
}

static std::vector<Particle> cosmologyParticles()
{
    std::vector<Particle> particles;
    constexpr int side = 6;
    for (int z = 0; z < side; ++z) {
        for (int y = 0; y < side; ++y) {
            for (int x = 0; x < side; ++x) {
                const float fx = static_cast<float>(x) - 2.5f;
                const float fy = static_cast<float>(y) - 2.5f;
                const float fz = static_cast<float>(z) - 2.5f;
                particles.push_back(particle(Vector3(
                    fx + 0.1f * std::sin(fy), fy + 0.1f * std::sin(fz), fz + 0.1f * std::sin(fx))));
            }
        }
    }
    return particles;
}

static std::vector<Particle> orbitParticles()
{
    std::vector<Particle> particles;
    constexpr int count = 64;
    constexpr float radius = 3.0f;
    constexpr float speed = 0.1f;
    particles.reserve(count);
    for (int index = 0; index < count; ++index) {
        const float angle = 6.2831853f * static_cast<float>(index) / static_cast<float>(count);
        Particle value =
            particle(Vector3(radius * std::cos(angle), radius * std::sin(angle), 0.0f));
        value.setVelocity(Vector3(-speed * std::sin(angle), speed * std::cos(angle), 0.0f));
        particles.push_back(value);
    }
    return particles;
}

static std::vector<Vector3> pairwise(const std::vector<Particle>& particles,
                                     const ForceLawPolicy& policy)
{
    std::vector<Vector3> accelerations(particles.size());
    for (std::size_t target = 0; target < particles.size(); ++target) {
        for (std::size_t source = 0; source < particles.size(); ++source) {
            if (target == source)
                continue;
            const Vector3 delta = particles[source].getPosition() - particles[target].getPosition();
            const float distance2 = dot(delta, delta) + policy.softening * policy.softening;
            if (distance2 <= policy.minDistance2)
                continue;
            const float inverseDistance = 1.0f / std::sqrt(distance2);
            accelerations[target] += delta * (particles[source].getMass() * inverseDistance *
                                              inverseDistance * inverseDistance);
        }
    }
    return accelerations;
}

static void expectQualified(const std::vector<Particle>& particles)
{
    const ForceLawPolicy policy = resolveForceLawPolicy(0.5f, 0.05f, 1.0e-4f, 1.0e-12f, 0.05f);
    bltzr_fmm::FmmWorkspace workspace;
    bltzr_fmm::configure(workspace, 16, 0.5f);
    std::vector<Vector3> approximate;
    ASSERT_TRUE(bltzr_fmm::computeForces(particles, policy, workspace, approximate));
    const bltzr_fmm::ForceErrorMetrics errors =
        bltzr_fmm::measureForceError(approximate, pairwise(particles, policy));
    SCOPED_TRACE(::testing::Message()
                 << "leaves=" << workspace.metrics.leafCount
                 << " depth=" << workspace.metrics.minLeafDepth << ':'
                 << workspace.metrics.maxLeafDepth << " m2l=" << workspace.metrics.m2lInteractions
                 << " p2p=" << workspace.metrics.p2pInteractions << " l2=" << errors.relativeL2
                 << " linf=" << errors.relativeLinf << " p99=" << errors.relativeP99);
    EXPECT_TRUE(workspace.metrics.finite);
    EXPECT_TRUE(workspace.metrics.balancedTwoToOne);
    EXPECT_GT(workspace.metrics.leafCount, 1u);
    EXPECT_GT(workspace.metrics.m2lInteractions, 0u);
    EXPECT_TRUE(errors.finite);
    EXPECT_LT(errors.relativeL2, 0.08);
    EXPECT_LT(errors.relativeLinf, 0.20);
    EXPECT_LT(errors.relativeP99, 0.30);
}

static bool advanceLeapfrog(std::vector<Particle>& particles, const ForceLawPolicy& policy,
                            bltzr_fmm::FmmWorkspace& workspace, float dt)
{
    std::vector<Vector3> acceleration;
    if (!bltzr_fmm::computeForces(particles, policy, workspace, acceleration))
        return false;
    for (std::size_t index = 0; index < particles.size(); ++index) {
        const Vector3 halfVelocity =
            particles[index].getVelocity() + acceleration[index] * (0.5f * dt);
        particles[index].setVelocity(halfVelocity);
        particles[index].setPosition(particles[index].getPosition() + halfVelocity * dt);
    }
    if (!bltzr_fmm::computeForces(particles, policy, workspace, acceleration))
        return false;
    for (std::size_t index = 0; index < particles.size(); ++index)
        particles[index].setVelocity(particles[index].getVelocity() +
                                     acceleration[index] * (0.5f * dt));
    return true;
}
TEST(PhysicsTest, TST_UNT_FMM_002_AdaptiveTreeQualifiesPlummerSphere)
{
    expectQualified(plummerParticles(192u));
}

TEST(PhysicsTest, TST_UNT_FMM_003_AdaptiveTreeQualifiesGalaxyCollision)
{
    expectQualified(collisionParticles(192u));
}

TEST(PhysicsTest, TST_UNT_FMM_004_AdaptiveTreeQualifiesPerturbedCosmology)
{
    expectQualified(cosmologyParticles());
}

TEST(PhysicsTest, TST_UNT_FMM_005_LeapfrogPreservesIsolatedInvariants)
{
    const ForceLawPolicy policy = resolveForceLawPolicy(0.5f, 0.05f, 1.0e-4f, 1.0e-12f, 0.05f);
    std::vector<Particle> particles = orbitParticles();
    const bltzr_fmm::StateInvariantMetrics initial =
        bltzr_fmm::measureStateInvariants(particles, policy);
    bltzr_fmm::FmmWorkspace workspace;
    bltzr_fmm::configure(workspace, 8, 0.45f);
    for (int step = 0; step < 64; ++step)
        ASSERT_TRUE(advanceLeapfrog(particles, policy, workspace, 0.001f));
    const bltzr_fmm::StateInvariantMetrics final =
        bltzr_fmm::measureStateInvariants(particles, policy);
    const double energyDrift = std::fabs(final.totalEnergy - initial.totalEnergy) /
                               std::max(std::fabs(initial.totalEnergy), 1.0e-12);
    const double angularDrift =
        std::sqrt(static_cast<double>(dot(final.angularMomentum - initial.angularMomentum,
                                          final.angularMomentum - initial.angularMomentum))) /
        std::max(
            std::sqrt(static_cast<double>(dot(initial.angularMomentum, initial.angularMomentum))),
            1.0e-12);
    const double momentumScale = 64.0 * 0.01 * 0.1;
    const double momentumDrift =
        std::sqrt(static_cast<double>(dot(final.linearMomentum, final.linearMomentum))) /
        momentumScale;
    EXPECT_TRUE(initial.finite);
    EXPECT_TRUE(final.finite);
    EXPECT_LT(energyDrift, 0.02);
    EXPECT_LT(angularDrift, 0.02);
    EXPECT_LT(momentumDrift, 0.02);
}
