/*
 * @file tests/unit/physics/fmm.cpp
 * @brief Deterministic qualification for the order-two CPU fast multipole solver.
 */

#include "physics/fmm/FmmCpu.hpp"
#include "physics/core/ForceLawPolicy.hpp"
#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

static float nextUnit(std::uint32_t& state)
{
    state = state * 1664525u + 1013904223u;
    return static_cast<float>(state >> 8u) / 16777215.0f;
}

static std::vector<Particle> buildParticles()
{
    std::uint32_t state = 24680u;
    std::vector<Particle> particles(512u);
    for (Particle& particle : particles) {
        particle.setPosition(Vector3(20.0f * nextUnit(state) - 10.0f,
                                     20.0f * nextUnit(state) - 10.0f,
                                     20.0f * nextUnit(state) - 10.0f));
        particle.setMass(0.005f + 0.01f * nextUnit(state));
    }
    return particles;
}

static Vector3 pairwiseForce(const std::vector<Particle>& particles, std::size_t target,
                             const ForceLawPolicy& policy)
{
    Vector3 acceleration;
    const Vector3 position = particles[target].getPosition();
    for (std::size_t source = 0u; source < particles.size(); ++source) {
        if (source == target)
            continue;
        const Vector3 delta = particles[source].getPosition() - position;
        const float distance2 = dot(delta, delta) + policy.softening * policy.softening;
        if (distance2 > policy.minDistance2) {
            const float inverseDistance = 1.0f / std::sqrt(distance2);
            acceleration += delta * (particles[source].getMass() * inverseDistance *
                                     inverseDistance * inverseDistance);
        }
    }
    return acceleration;
}
TEST(PhysicsTest, TST_UNT_FMM_001_OrderTwoCpuFmmMatchesPairwiseReference)
{
    const std::vector<Particle> particles = buildParticles();
    const ForceLawPolicy policy = resolveForceLawPolicy(0.5f, 0.05f, 1.0e-4f, 1.0e-12f, 0.05f);
    bltzr_fmm::FmmWorkspace workspace;
    std::vector<Vector3> fmm;
    ASSERT_TRUE(bltzr_fmm::computeForces(particles, policy, workspace, fmm));
    ASSERT_EQ(fmm.size(), particles.size());

    std::vector<Vector3> reference(particles.size());
    for (std::size_t index = 0u; index < particles.size(); ++index) {
        reference[index] = pairwiseForce(particles, index, policy);
    }
    const bltzr_fmm::ForceErrorMetrics metrics = bltzr_fmm::measureForceError(fmm, reference);
    EXPECT_TRUE(metrics.finite);
    EXPECT_LT(metrics.relativeL2, 0.03);
}
