// File: tests/unit/ui/qt_octree_overlay.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "ui/OctreeOverlay.hpp"
#include <gtest/gtest.h>
#include <vector>
namespace grav_test_qt_ui {
/// Description: Executes the TEST operation.
TEST(QtUiLogicTest, TST_UNT_UI_009_OctreeOverlayBuildRespectsDepthLimit)
{
    const std::vector<RenderParticle> particles = {
        {-1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f}, {1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f},
        {-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f},  {1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f},
        {-1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f},  {1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
        {-1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},   {1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f}};
    const std::vector<grav_qt::OctreeOverlayNode> depthZero =
        /// Description: Executes the build operation.
        grav_qt::OctreeOverlay::build(particles, 0);
    const std::vector<grav_qt::OctreeOverlayNode> depthOne =
        /// Description: Executes the build operation.
        grav_qt::OctreeOverlay::build(particles, 1);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(depthZero.size(), 1u);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(depthOne.size(), 9u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(depthZero.front().depth, 0);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(depthOne.front().depth, 0);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(depthOne.back().depth, 1);
}
} // namespace grav_test_qt_ui
