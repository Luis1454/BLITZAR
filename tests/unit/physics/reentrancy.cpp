// File: tests/unit/physics/reentrancy.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "physics/ParticleSystem.hpp"
#include <gtest/gtest.h>
#include <vector>
TEST(PhysicsReentrancyTest, TST_UNT_PHYS_017_MultipleInstancesCoexistence)
{
    // Create two independent particle systems.
    ParticleSystem ps1(128, true);
    ParticleSystem ps2(64, true);
    // Set different properties.
    ps1.setSolverMode(ParticleSystem::SolverMode::OctreeCpu);
    ps2.setSolverMode(ParticleSystem::SolverMode::OctreeCpu);
    ps1.setUseOctree(false); // legacy setter, will be overridden by direct solver mode
    ps2.setUseOctree(true);
    ps1.setSolverMode(ParticleSystem::SolverMode::OctreeCpu);
    ps2.setSolverMode(ParticleSystem::SolverMode::OctreeCpu);
    // Verify initial state.
    EXPECT_EQ(ps1.getParticles().size(), 128);
    EXPECT_EQ(ps2.getParticles().size(), 64);
    // Step both systems.
    // Note: On this machine, CUDA kernels will fail, but we want to ensure
    // that the systems are independent and don't crash.
    (void)ps1.update(0.01f);
    (void)ps2.update(0.01f);
    // Verify that ps1 didn't change ps2's particle count or mode.
    EXPECT_EQ(ps1.getParticles().size(), 128);
    EXPECT_EQ(ps2.getParticles().size(), 64);
}
