#include "ui/OctreeOverlay.hpp"
#include <gtest/gtest.h>
#include <vector>
namespace grav_test_qt_ui {
TEST(QtUiLogicTest, TST_UNT_UI_009_OctreeOverlayBuildRespectsDepthLimit)
{
    const std::vector<RenderParticle> particles = {
        {-1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f}, {1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f},
        {-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f},  {1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f},
        {-1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f},  {1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
        {-1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},   {1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f}};
    const std::vector<grav_qt::OctreeOverlayNode> depthZero =
        grav_qt::OctreeOverlay::build(particles, 0);
    const std::vector<grav_qt::OctreeOverlayNode> depthOne =
        grav_qt::OctreeOverlay::build(particles, 1);
    ASSERT_EQ(depthZero.size(), 1u);
    ASSERT_EQ(depthOne.size(), 9u);
    EXPECT_EQ(depthZero.front().depth, 0);
    EXPECT_EQ(depthOne.front().depth, 0);
    EXPECT_EQ(depthOne.back().depth, 1);
}
} // namespace grav_test_qt_ui
