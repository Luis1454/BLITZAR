/*
 * @file tests/unit/physics/force_law_policy.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "physics/ForceLawPolicy.hpp"
#include <gtest/gtest.h>

TEST(PhysicsTest, TST_UNT_PHYS_021_ForceLawPolicyClampsConfiguredThresholds)
{
    const ForceLawPolicy policy = resolveForceLawPolicy(0.01f, 1.0e-6f, 2.0e-4f, -1.0f, 0.001f);
    EXPECT_FLOAT_EQ(policy.theta, 0.05f);
    EXPECT_FLOAT_EQ(policy.softening, 2.0e-4f);
    EXPECT_FLOAT_EQ(policy.minSoftening, 2.0e-4f);
    EXPECT_FLOAT_EQ(policy.minDistance2, 0.0f);
    EXPECT_FLOAT_EQ(policy.minTheta, 0.05f);
}
