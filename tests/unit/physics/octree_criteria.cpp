#include "config/SimulationConfig.hpp"
#include "physics/Octree.hpp"
#include "physics/Particle.hpp"
#include "server/SimulationServer.hpp"
#include "tests/support/physics_test_utils.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

namespace grav_test_octree_criteria {

Vector3 pairwiseAcceleration(
    const std::vector<Particle> &particles,
    std::size_t selfIndex,
    float softening,
    float minSoftening,
    float minDistance2)
{
    Vector3 total(0.0f, 0.0f, 0.0f);
    const Vector3 selfPosition = particles[selfIndex].getPosition();
    const float safeSoftening = std::max(softening, minSoftening);
    for (std::size_t i = 0; i < particles.size(); ++i) {
        if (i == selfIndex) {
            continue;
        }
        const Vector3 delta = particles[i].getPosition() - selfPosition;
        const float dist2 = dot(delta, delta) + safeSoftening * safeSoftening;
        if (dist2 <= minDistance2) {
            continue;
        }
        const float invDist = 1.0f / std::sqrt(dist2);
        const float invDist3 = invDist * invDist * invDist;
        total += delta * (particles[i].getMass() * invDist3);
    }
    return total;
}

float magnitude(Vector3 value)
{
    return std::sqrt(dot(value, value));
}

std::vector<Particle> buildCriterionScenario()
{
    std::vector<Particle> particles(9);
    particles[0].setPosition(Vector3(0.0f, 0.0f, 0.0f));
    particles[0].setVelocity(Vector3(0.0f, 0.0f, 0.0f));
    particles[0].setMass(1.0f);

    const std::vector<Vector3> remotePositions{
        Vector3(6.0f, -4.0f, -4.0f),
        Vector3(6.0f, 4.0f, -4.0f),
        Vector3(6.0f, -4.0f, 4.0f),
        Vector3(6.0f, 4.0f, 4.0f),
        Vector3(14.0f, -4.0f, -4.0f),
        Vector3(14.0f, 4.0f, -4.0f),
        Vector3(14.0f, -4.0f, 4.0f),
        Vector3(14.0f, 4.0f, 4.0f)
    };
    for (std::size_t i = 0; i < remotePositions.size(); ++i) {
        particles[i + 1].setPosition(remotePositions[i]);
        particles[i + 1].setVelocity(Vector3(0.0f, 0.0f, 0.0f));
        particles[i + 1].setMass(0.5f + static_cast<float>(i) * 0.1f);
    }
    return particles;
}

std::filesystem::path writeOctreeServerConfig()
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / ("gravity_octree_criteria_server_" + std::to_string(stamp) + ".ini");
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
    EXPECT_TRUE(saved);
    return path;
}

} // namespace grav_test_octree_criteria

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
    const Vector3 reference = grav_test_octree_criteria::pairwiseAcceleration(
        particles,
        0u,
        softening,
        minSoftening,
        minDistance2);
    const Vector3 comForce = octree.computeForceOn(
        particles[0],
        0u,
        theta,
        softening,
        minSoftening,
        minDistance2,
        minTheta,
        OctreeOpeningCriterion::CenterOfMass);
    const Vector3 boundsForce = octree.computeForceOn(
        particles[0],
        0u,
        theta,
        softening,
        minSoftening,
        minDistance2,
        minTheta,
        OctreeOpeningCriterion::Bounds);

    const float comError = grav_test_octree_criteria::magnitude(reference - comForce);
    const float boundsError = grav_test_octree_criteria::magnitude(reference - boundsForce);

    EXPECT_LE(boundsError, comError + 1.0e-6f);
}

TEST(PhysicsTest, TST_UNT_RUNT_006_ServerLogsOctreeCriterionAndAutoThetaMetrics)
{
    const std::filesystem::path path = grav_test_octree_criteria::writeOctreeServerConfig();
    SimulationServer server(path.string());
    server.setPaused(true);

    server.start();

    std::vector<RenderParticle> snapshot;
    ASSERT_TRUE(testsupport::waitForConsumedSnapshot(server, snapshot, 4000));
    server.stepOnce();
    ASSERT_TRUE(testsupport::waitForStepCount(server, 1u, 4000));
    const SimulationConfig runtimeConfig = server.getRuntimeConfig();
    const SimulationStats stats = server.getStats();
    server.stop();

    EXPECT_EQ(runtimeConfig.octreeOpeningCriterion, "bounds");
    EXPECT_TRUE(runtimeConfig.octreeThetaAutoTune);
    EXPECT_FLOAT_EQ(runtimeConfig.octreeThetaAutoMin, 0.3f);
    EXPECT_FLOAT_EQ(runtimeConfig.octreeThetaAutoMax, 0.9f);
    EXPECT_GE(stats.serverFps, 0.0f);
    EXPECT_TRUE(std::isfinite(stats.energyDriftPct));

    std::error_code ec;
    std::filesystem::remove(path, ec);
}
