/*
 * @file tests/unit/ui/qt_octree_overlay.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "widgets/overlays/Octree.hpp"
#include <gtest/gtest.h>
#include <vector>

namespace bltzr_test_qt_ui {
TEST(QtUiLogicTest, TST_UNT_UI_009_OctreeOverlayBuildRespectsDepthLimit)
{
    const std::vector<RenderParticle> particles = {
        {-1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f}, {1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f},
        {-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f},  {1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f},
        {-1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f},  {1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
        {-1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},   {1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f}};
    const std::vector<bltzr_qt::OctreeNode> depthZero =
        bltzr_qt::Octree::build(particles, 0);
    const std::vector<bltzr_qt::OctreeNode> depthOne =
        bltzr_qt::Octree::build(particles, 1);
    ASSERT_EQ(depthZero.size(), 1u);
    ASSERT_EQ(depthOne.size(), 9u);
    EXPECT_EQ(depthZero.front().depth, 0);
    EXPECT_EQ(depthOne.front().depth, 0);
    EXPECT_EQ(depthOne.back().depth, 1);
}
} // namespace bltzr_test_qt_ui
