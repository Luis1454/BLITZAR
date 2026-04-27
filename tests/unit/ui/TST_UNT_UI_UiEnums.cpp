// @file tests/unit/ui/TST_UNT_UI_UiEnums.cpp
// @brief Unit tests for UI enum converters

#include "ui/UiEnums.hpp"
#include <gtest/gtest.h>

using namespace grav_qt;

TEST(UiEnumsTest, SolverConverters)
{
    EXPECT_EQ(to_string(Solver::PairwiseCuda), "pairwise_cuda");
    EXPECT_EQ(to_string(Solver::OctreeGpu), "octree_gpu");
    EXPECT_EQ(to_string(Solver::OctreeCpu), "octree_cpu");
    EXPECT_EQ(solver_from_string("pairwise_cuda"), Solver::PairwiseCuda);
    EXPECT_EQ(solver_from_string("octree_gpu"), Solver::OctreeGpu);
}

TEST(UiEnumsTest, IntegratorConverters)
{
    EXPECT_EQ(to_string(Integrator::Euler), "euler");
    EXPECT_EQ(to_string(Integrator::Rk4), "rk4");
    EXPECT_EQ(integrator_from_string("rk4"), Integrator::Rk4);
}

TEST(UiEnumsTest, PerformanceConverters)
{
    EXPECT_EQ(to_string(PerformanceProfile::Interactive), "interactive");
    EXPECT_EQ(to_string(PerformanceProfile::Balanced), "balanced");
    EXPECT_EQ(performance_from_string("quality"), PerformanceProfile::Quality);
    EXPECT_EQ(performance_from_string("unknown"), PerformanceProfile::Interactive);
}
