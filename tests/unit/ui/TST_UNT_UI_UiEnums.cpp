/*
 * @file tests/unit/ui/TST_UNT_UI_UiEnums.cpp
 * @brief Unit tests for UI enum converters.
 */

#include "ui/UiEnums.hpp"
#include <gtest/gtest.h>

TEST(UiEnumsTest, SolverConverters)
{
    EXPECT_EQ(bltzr_qt::to_string(bltzr_qt::Solver::PairwiseCuda), "pairwise_cuda");
    EXPECT_EQ(bltzr_qt::to_string(bltzr_qt::Solver::OctreeGpu), "octree_gpu");
    EXPECT_EQ(bltzr_qt::to_string(bltzr_qt::Solver::OctreeCpu), "octree_cpu");
    EXPECT_EQ(bltzr_qt::solver_from_string("pairwise_cuda"), bltzr_qt::Solver::PairwiseCuda);
    EXPECT_EQ(bltzr_qt::solver_from_string("octree_gpu"), bltzr_qt::Solver::OctreeGpu);
}

TEST(UiEnumsTest, IntegratorConverters)
{
    EXPECT_EQ(bltzr_qt::to_string(bltzr_qt::Integrator::Euler), "euler");
    EXPECT_EQ(bltzr_qt::to_string(bltzr_qt::Integrator::Rk4), "rk4");
    EXPECT_EQ(bltzr_qt::integrator_from_string("rk4"), bltzr_qt::Integrator::Rk4);
}

TEST(UiEnumsTest, PerformanceConverters)
{
    EXPECT_EQ(bltzr_qt::to_string(bltzr_qt::PerformanceProfile::Interactive), "interactive");
    EXPECT_EQ(bltzr_qt::to_string(bltzr_qt::PerformanceProfile::Balanced), "balanced");
    EXPECT_EQ(bltzr_qt::performance_from_string("quality"), bltzr_qt::PerformanceProfile::Quality);
    EXPECT_EQ(bltzr_qt::performance_from_string("unknown"),
              bltzr_qt::PerformanceProfile::Interactive);
}
