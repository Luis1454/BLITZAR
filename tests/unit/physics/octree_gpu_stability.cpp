// File: tests/unit/physics/octree_gpu_stability.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

/*
 * Test: octree_gpu_stability
 * Responsibility: Verify octree_gpu solver does not hang after step 1 (issue #386)
 * 
 * TST_PHY_OctreeGpuStabilitySmallBatch: 100 particles, octree_gpu, 10 steps, complete < 1s
 */

#include "tests/support/physics_scenario.hpp"

#include <gtest/gtest.h>
#include <chrono>
#include <string>

namespace testsupport {

// TST_PHY_OctreeGpuStabilitySmallBatch
TEST(OctreeGpuStability, SmallBatch_100ParticlesMultiStep)
{
    // Regression test for GPU hang at step 1 with octree_gpu (issue #386)
    // Verify 100 particles complete 10 steps without hang in < 10s
    
    const auto testStart = std::chrono::steady_clock::now();
    
    ScenarioConfig cfg;
    cfg.particleCount = 100u;
    cfg.steps = 10u;
    cfg.solver = "octree_gpu";
    cfg.integrator = "rk4";
    cfg.dt = 0.001f;
    cfg.octreeTheta = 0.5f;
    cfg.octreeSoftening = 0.01f;
    cfg.stepTimeoutMs = 5000;
    cfg.snapshotTimeoutMs = 5000;
    
    // Prepare scenario with generated particles
    std::string prepError;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(prepareGeneratedCalibrationScenario("three_body", cfg, prepError))
        << "Failed to prepare scenario: " << prepError;
    
    ScenarioResult result;
    std::string error;
    
    // Run scenario and measure time
    const auto runStart = std::chrono::steady_clock::now();
    const bool success = runScenario(cfg, result, error);
    const auto runEnd = std::chrono::steady_clock::now();
    
    const float elapsedSeconds = std::chrono::duration<float>(runEnd - runStart).count();
    
    // Verify: simulation must complete successfully
    EXPECT_TRUE(success) << "Scenario run failed: " << error 
                         << " (GPU hang or solver error at step 1?)";
    
    // Verify: 10 steps with 100 particles should complete in reasonable time
    EXPECT_LT(elapsedSeconds, 10.0f)
        << "Octree_gpu took " << elapsedSeconds
        << "s for 10 steps with 100 particles (expected < 10s)";
    
    const auto testEnd = std::chrono::steady_clock::now();
    const float totalElapsed = std::chrono::duration<float>(testEnd - testStart).count();
    GTEST_LOG_(INFO) << "[OctreeGpuStability] Completed " << cfg.steps << " steps in "
                     << elapsedSeconds << "s (test total: " << totalElapsed << "s)";
}

}  // namespace testsupport
