/*
 * @file engine/types/tests/simulation_types.cpp
 * @brief Shared simulation type invariants.
 */

#include "types/simulation/TypSimulationTypes.hpp"

#include <gtest/gtest.h>

TEST(TypesTest, InitialStateDefaultsAreDeterministic)
{
    const InitialStateConfig config;
    EXPECT_EQ(config.mode, "disk_orbit");
    EXPECT_EQ(config.seed, 42u);
    EXPECT_TRUE(config.includeCentralBody);
}
