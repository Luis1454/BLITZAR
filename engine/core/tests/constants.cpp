/*
 * @file engine/core/tests/constants.cpp
 * @brief Shared engine constant invariants.
 */

#include "core/constants/FndConstants.hpp"

#include <gtest/gtest.h>

TEST(CoreTest, SimulationDefaultsAreOrdered)
{
    EXPECT_GT(kDefaultSimulationDt, kMinSimulationDt);
    EXPECT_LT(kDefaultSimulationDt, kMaxStableInteractiveDt);
    EXPECT_LT(kPhysicsMinTheta, kPhysicsMaxTheta);
}
