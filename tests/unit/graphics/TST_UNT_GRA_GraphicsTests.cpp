// File: tests/unit/graphics/TST_UNT_GRA_GraphicsTests.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "graphics/ColorPipeline.hpp"
#include "graphics/ViewMath.hpp"
#include <gtest/gtest.h>
namespace grav {
constexpr float kIsoYawForTest = 0.78539816339f;
constexpr float kIsoPitchForTest = 0.61547970867f;
/// Description: Executes the TEST operation.
TEST(ViewMathTest, TST_UNT_GRA_001_ProjectXYIsIdentity)
{
    RenderParticle p;
    p.x = 10.0f;
    p.y = 20.0f;
    p.z = 30.0f;
    CameraState cam{0.0f, 0.0f, 0.0f};
    auto pp = projectParticle(p, ViewMode::XY, cam);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(pp.valid);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(pp.x, 10.0f, 1e-4f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(pp.y, 20.0f, 1e-4f);
}
/// Description: Executes the TEST operation.
TEST(ViewMathTest, TST_UNT_GRA_002_ProjectXZSwapsYZ)
{
    RenderParticle p;
    p.x = 10.0f;
    p.y = 20.0f;
    p.z = 30.0f;
    CameraState cam{0.0f, 0.0f, 0.0f};
    auto pp = projectParticle(p, ViewMode::XZ, cam);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(pp.valid);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(pp.x, 10.0f, 1e-4f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(pp.y, 30.0f, 1e-4f);
}
/// Description: Executes the TEST operation.
TEST(ViewMathTest, TST_UNT_GRA_004_ProjectYZSwapsAxes)
{
    RenderParticle p;
    p.x = 11.0f;
    p.y = 22.0f;
    p.z = 33.0f;
    CameraState cam{0.0f, 0.0f, 0.0f};
    const ProjectedPoint pp = projectParticle(p, ViewMode::YZ, cam);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(pp.valid);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(pp.x, 22.0f, 1e-4f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(pp.y, 33.0f, 1e-4f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(pp.depth, 11.0f, 1e-4f);
}
/// Description: Executes the TEST operation.
TEST(ViewMathTest, TST_UNT_GRA_005_PerspectiveRejectsNearPlane)
{
    RenderParticle p;
    p.x = 2.0f;
    p.y = 3.0f;
    p.z = 40.0f;
    CameraState cam{-kIsoYawForTest, -kIsoPitchForTest, 0.0f};
    const ProjectedPoint pp = projectParticle(p, ViewMode::Perspective, cam);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(pp.valid);
}
/// Description: Executes the TEST operation.
TEST(ViewMathTest, TST_UNT_GRA_006_PerspectiveRejectsInvalidScaleRange)
{
    CameraState cam{-kIsoYawForTest, -kIsoPitchForTest, 0.0f};
    RenderParticle nearPlane;
    nearPlane.x = 1.0f;
    nearPlane.y = 1.0f;
    nearPlane.z = 39.99f;
    const ProjectedPoint tooLarge = projectParticle(nearPlane, ViewMode::Perspective, cam);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(tooLarge.valid);
    RenderParticle behindCamera;
    behindCamera.x = 1.0f;
    behindCamera.y = 1.0f;
    behindCamera.z = 120.0f;
    const ProjectedPoint negative = projectParticle(behindCamera, ViewMode::Perspective, cam);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(negative.valid);
}
/// Description: Executes the TEST operation.
TEST(ViewMathTest, TST_UNT_GRA_007_PerspectiveScalesWhenValid)
{
    RenderParticle p;
    p.x = 5.0f;
    p.y = 7.0f;
    p.z = 20.0f;
    CameraState cam{-kIsoYawForTest, -kIsoPitchForTest, 0.0f};
    const ProjectedPoint pp = projectParticle(p, ViewMode::Perspective, cam);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(pp.valid);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(pp.x, 10.0f, 1e-3f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(pp.y, 14.0f, 1e-3f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(pp.depth, 20.0f, 1e-3f);
}
/// Description: Executes the TEST operation.
TEST(ViewMathTest, TST_UNT_GRA_008_ComputeGimbalClampsBoundsAndPickAxis)
{
    CameraState cam{0.0f, 0.0f, 0.0f};
    const Rect2D smallViewport{0.0f, 0.0f, 100.0f, 100.0f};
    const GimbalOverlay small = computeGimbal(smallViewport, ViewMode::XY, cam);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(small.bounds.width, 54.0f, 1e-4f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(small.bounds.height, 54.0f, 1e-4f);
    const Rect2D largeViewport{0.0f, 0.0f, 1000.0f, 1000.0f};
    const GimbalOverlay large = computeGimbal(largeViewport, ViewMode::Iso, cam);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(large.bounds.width, 94.0f, 1e-4f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(large.bounds.height, 94.0f, 1e-4f);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(pickGimbalAxis(large, large.handles[0]), GimbalAxis::X);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(pickGimbalAxis(large, large.handles[1]), GimbalAxis::Y);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(pickGimbalAxis(large, large.handles[2]), GimbalAxis::Z);
    const Point2D farPoint{large.center.x + 200.0f, large.center.y + 200.0f};
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(pickGimbalAxis(large, farPoint), GimbalAxis::None);
}
/// Description: Executes the TEST operation.
TEST(ColorPipelineTest, TST_UNT_GRA_003_HeavyBodyDetect)
{
    RenderParticle p1;
    p1.mass = 50.0f;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(isHeavyBody(p1));
    RenderParticle p2;
    p2.mass = 150.0f;
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(isHeavyBody(p2));
}
/// Description: Executes the TEST operation.
TEST(ColorPipelineTest, TST_UNT_GRA_009_HeavyBodyColorClampsLuminosity)
{
    const ColorRGBA low = heavyBodyColor(-5);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(low.r, 255);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(low.g, 90);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(low.b, 90);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(low.a, 0);
    const ColorRGBA high = heavyBodyColor(300);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(high.a, 255);
}
/// Description: Executes the TEST operation.
TEST(ColorPipelineTest, TST_UNT_GRA_010_ParticleRampColorFastClampsBinsAndAlpha)
{
    RenderParticle cold;
    cold.temperature = -2.0f;
    cold.pressureNorm = -1.0f;
    const ColorRGBA coldColor = particleRampColorFast(cold, 0.1f, 0.1f, 0);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(coldColor.r, 56);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(coldColor.g, 105);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(coldColor.b, 255);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(coldColor.a, 6);
    RenderParticle hot;
    hot.temperature = 9999.0f;
    hot.pressureNorm = 9999.0f;
    const ColorRGBA hotColor = particleRampColorFast(hot, 1.0f, 1.0f, 500);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(hotColor.r, 255);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(hotColor.g, 76);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(hotColor.b, 66);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(hotColor.a, 255);
}
/// Description: Executes the TEST operation.
TEST(ColorPipelineTest, TST_UNT_GRA_011_UpdateAdaptiveScalesSupportsRiseAndFall)
{
    float adaptiveTemperatureScale = 1.0f;
    float adaptivePressureScale = 1.0f;
    const std::vector<RenderParticle> empty{};
    /// Description: Executes the updateAdaptiveScales operation.
    updateAdaptiveScales(empty, adaptiveTemperatureScale, adaptivePressureScale);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(adaptiveTemperatureScale, 0.97f, 1e-5f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(adaptivePressureScale, 0.97f, 1e-5f);
    RenderParticle p;
    p.temperature = 4.0f;
    p.pressureNorm = 3.0f;
    std::vector<RenderParticle> snapshot{p};
    /// Description: Executes the updateAdaptiveScales operation.
    updateAdaptiveScales(snapshot, adaptiveTemperatureScale, adaptivePressureScale);
    /// Description: Executes the EXPECT_GT operation.
    EXPECT_GT(adaptiveTemperatureScale, 0.97f);
    /// Description: Executes the EXPECT_GT operation.
    EXPECT_GT(adaptivePressureScale, 0.97f);
}
/// Description: Executes the TEST operation.
TEST(ColorPipelineTest, TST_UNT_GRA_012_UpdateAdaptiveScalesIgnoresNegativeObservations)
{
    float adaptiveTemperatureScale = 0.1f;
    float adaptivePressureScale = 0.1f;
    RenderParticle p;
    p.temperature = -10.0f;
    p.pressureNorm = -5.0f;
    std::vector<RenderParticle> snapshot{p};
    /// Description: Executes the updateAdaptiveScales operation.
    updateAdaptiveScales(snapshot, adaptiveTemperatureScale, adaptivePressureScale);
    /// Description: Executes the EXPECT_GT operation.
    EXPECT_GT(adaptiveTemperatureScale, 0.1f);
    /// Description: Executes the EXPECT_GT operation.
    EXPECT_GT(adaptivePressureScale, 0.1f);
    /// Description: Executes the EXPECT_GE operation.
    EXPECT_GE(adaptiveTemperatureScale, 0.148f);
    /// Description: Executes the EXPECT_GE operation.
    EXPECT_GE(adaptivePressureScale, 0.148f);
}
} // namespace grav
