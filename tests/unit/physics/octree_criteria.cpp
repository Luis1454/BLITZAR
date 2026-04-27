// File: tests/unit/physics/octree_criteria.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "config/SimulationConfig.hpp"
#include "physics/ForceLawPolicy.hpp"
#include "physics/Octree.hpp"
#include "physics/Particle.hpp"
#include "server/SimulationServer.hpp"
#include "tests/support/physics_test_utils.hpp"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>
namespace grav_test_octree_criteria {
Vector3 pairwiseAcceleration(const std::vector<Particle>& particles, std::size_t selfIndex,
                             const ForceLawPolicy& policy)
{
    /// Description: Executes the total operation.
    Vector3 total(0.0f, 0.0f, 0.0f);
    const Vector3 selfPosition = particles[selfIndex].getPosition();
    for (std::size_t i = 0; i < particles.size(); ++i) {
        if (i == selfIndex)
            continue;
        const Vector3 delta = particles[i].getPosition() - selfPosition;
        const float dist2 = dot(delta, delta) + policy.softening * policy.softening;
        if (dist2 <= policy.minDistance2)
            continue;
        const float invDist = 1.0f / std::sqrt(dist2);
        const float invDist3 = invDist * invDist * invDist;
        total += delta * (particles[i].getMass() * invDist3);
    }
    return total;
}
/// Description: Executes the magnitude operation.
float magnitude(Vector3 value)
{
    return std::sqrt(dot(value, value));
}
/// Description: Executes the buildCriterionScenario operation.
std::vector<Particle> buildCriterionScenario()
{
    /// Description: Executes the particles operation.
    std::vector<Particle> particles(9);
    particles[0].setPosition(Vector3(0.0f, 0.0f, 0.0f));
    particles[0].setVelocity(Vector3(0.0f, 0.0f, 0.0f));
    particles[0].setMass(1.0f);
    const std::vector<Vector3> remotePositions{
        Vector3(6.0f, -4.0f, -4.0f), Vector3(6.0f, 4.0f, -4.0f),   Vector3(6.0f, -4.0f, 4.0f),
        Vector3(6.0f, 4.0f, 4.0f),   Vector3(14.0f, -4.0f, -4.0f), Vector3(14.0f, 4.0f, -4.0f),
        Vector3(14.0f, -4.0f, 4.0f), Vector3(14.0f, 4.0f, 4.0f)};
    for (std::size_t i = 0; i < remotePositions.size(); ++i) {
        particles[i + 1].setPosition(remotePositions[i]);
        particles[i + 1].setVelocity(Vector3(0.0f, 0.0f, 0.0f));
        particles[i + 1].setMass(0.5f + static_cast<float>(i) * 0.1f);
    }
    return particles;
}
/// Description: Executes the writeOctreeServerConfig operation.
std::filesystem::path writeOctreeServerConfig()
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("gravity_octree_criteria_server_" + std::to_string(stamp) + ".ini");
    SimulationConfig config = SimulationConfig::defaults();
    config.particleCount = 16u;
    config.dt = 0.005f;
    config.simulationProfile = "manual_override";
    config.solver = "octree_cpu";
    config.integrator = "euler";
    config.performanceProfile = "balanced";
    config.octreeTheta = 0.7f;
    config.octreeSoftening = 0.02f;
    config.octreeOpeningCriterion = "bounds";
    config.octreeThetaAutoTune = true;
    config.octreeThetaAutoMin = 0.3f;
    config.octreeThetaAutoMax = 0.9f;
    config.energyMeasureEverySteps = 1u;
    config.energySampleLimit = 64u;
    const bool saved = config.save(path.string());
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(saved);
    return path;
}
} // namespace grav_test_octree_criteria
/// Description: Executes the TEST operation.
TEST(PhysicsTest, TST_UNT_PHYS_019_BoundsCriterionIsMoreConservativeThanComCriterion)
{
    const std::vector<Particle> particles = grav_test_octree_criteria::buildCriterionScenario();
    Octree octree;
    octree.build(particles);
    const float theta = 0.9f;
    const float softening = 0.01f;
    const float minSoftening = 1.0e-4f;
    const float minDistance2 = 1.0e-12f;
    const float minTheta = 0.05f;
    const ForceLawPolicy policy =
        /// Description: Executes the resolveForceLawPolicy operation.
        resolveForceLawPolicy(theta, softening, minSoftening, minDistance2, minTheta);
    const Vector3 reference =
        /// Description: Executes the pairwiseAcceleration operation.
        grav_test_octree_criteria::pairwiseAcceleration(particles, 0u, policy);
    const Vector3 comForce =
        octree.computeForceOn(particles[0], 0u, policy, OctreeOpeningCriterion::CenterOfMass);
    const Vector3 boundsForce =
        octree.computeForceOn(particles[0], 0u, policy, OctreeOpeningCriterion::Bounds);
    const float comError = grav_test_octree_criteria::magnitude(reference - comForce);
    const float boundsError = grav_test_octree_criteria::magnitude(reference - boundsForce);
    /// Description: Executes the EXPECT_LE operation.
    EXPECT_LE(boundsError, comError + 1.0e-6f);
}
/// Description: Executes the TEST operation.
TEST(PhysicsTest, TST_UNT_RUNT_006_ServerLogsOctreeCriterionAndAutoThetaMetrics)
{
    const std::filesystem::path path = grav_test_octree_criteria::writeOctreeServerConfig();
    /// Description: Executes the server operation.
    SimulationServer server(path.string());
    server.setPaused(true);
    server.start();
    std::vector<RenderParticle> snapshot;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(testsupport::waitForConsumedSnapshot(server, snapshot, 4000));
    server.stepOnce();
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(testsupport::waitForStepCount(server, 1u, 4000));
    const SimulationConfig runtimeConfig = server.getRuntimeConfig();
    const SimulationStats stats = server.getStats();
    server.stop();
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(runtimeConfig.octreeOpeningCriterion, "bounds");
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(runtimeConfig.octreeThetaAutoTune);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(runtimeConfig.octreeThetaAutoMin, 0.3f);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(runtimeConfig.octreeThetaAutoMax, 0.9f);
    /// Description: Executes the EXPECT_GE operation.
    EXPECT_GE(stats.serverFps, 0.0f);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(std::isfinite(stats.energyDriftPct));
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(path, ec);
}
