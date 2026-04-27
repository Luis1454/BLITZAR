/*
 * @file tests/unit/ui/TST_UNT_UI_UiEnums.cpp
 * @brief Unit tests for UI enum converters.
 */

#include "ui/UiEnums.hpp"
#include <gtest/gtest.h>

TEST(UiEnumsTest, SolverConverters)
{
    EXPECT_EQ(grav_qt::to_string(grav_qt::Solver::PairwiseCuda), "pairwise_cuda");
    EXPECT_EQ(grav_qt::to_string(grav_qt::Solver::OctreeGpu), "octree_gpu");
    EXPECT_EQ(grav_qt::to_string(grav_qt::Solver::OctreeCpu), "octree_cpu");
    EXPECT_EQ(grav_qt::solver_from_string("pairwise_cuda"), grav_qt::Solver::PairwiseCuda);
    EXPECT_EQ(grav_qt::solver_from_string("octree_gpu"), grav_qt::Solver::OctreeGpu);
}

TEST(UiEnumsTest, IntegratorConverters)
{
    EXPECT_EQ(grav_qt::to_string(grav_qt::Integrator::Euler), "euler");
    EXPECT_EQ(grav_qt::to_string(grav_qt::Integrator::Rk4), "rk4");
    EXPECT_EQ(grav_qt::integrator_from_string("rk4"), grav_qt::Integrator::Rk4);
}

TEST(UiEnumsTest, PerformanceConverters)
{
    EXPECT_EQ(grav_qt::to_string(grav_qt::PerformanceProfile::Interactive), "interactive");
    EXPECT_EQ(grav_qt::to_string(grav_qt::PerformanceProfile::Balanced), "balanced");
    EXPECT_EQ(grav_qt::performance_from_string("quality"), grav_qt::PerformanceProfile::Quality);
    EXPECT_EQ(grav_qt::performance_from_string("unknown"), grav_qt::PerformanceProfile::Interactive);
}
