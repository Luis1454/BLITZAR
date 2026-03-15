#include <gtest/gtest.h>
#include "graphics/ViewMath.hpp"
#include "graphics/ColorPipeline.hpp"

namespace grav {

TEST(ViewMathTest, TST_UNT_GRA_001_ProjectXYIsIdentity) {
    RenderParticle p;
    p.x = 10.0f;
    p.y = 20.0f;
    p.z = 30.0f;
    
    CameraState cam{0.0f, 0.0f, 0.0f};
    auto pp = projectParticle(p, ViewMode::XY, cam);
    
    EXPECT_TRUE(pp.valid);
    EXPECT_NEAR(pp.x, 10.0f, 1e-4f);
    EXPECT_NEAR(pp.y, 20.0f, 1e-4f);
}

TEST(ViewMathTest, TST_UNT_GRA_002_ProjectXZSwapsYZ) {
    RenderParticle p;
    p.x = 10.0f;
    p.y = 20.0f;
    p.z = 30.0f;
    
    CameraState cam{0.0f, 0.0f, 0.0f};
    auto pp = projectParticle(p, ViewMode::XZ, cam);
    
    EXPECT_TRUE(pp.valid);
    EXPECT_NEAR(pp.x, 10.0f, 1e-4f);
    EXPECT_NEAR(pp.y, 30.0f, 1e-4f);
}

TEST(ColorPipelineTest, TST_UNT_GRA_003_HeavyBodyDetect) {
    RenderParticle p1;
    p1.mass = 50.0f;
    EXPECT_FALSE(isHeavyBody(p1));
    
    RenderParticle p2;
    p2.mass = 150.0f;
    EXPECT_TRUE(isHeavyBody(p2));
}

} // namespace grav
