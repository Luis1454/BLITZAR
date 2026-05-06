/*
 * @file modules/qt/ui/particleView/particleRasterizer.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Particle rasterizer implementations.
 */

#include "ui/particleView/particleRasterizer.hpp"

namespace bltzr_qt::particleView {

void ParticleRasterizer::rasterizeParticles(const ParticleView* view,
                                            const std::vector<RenderParticle>& particles,
                                            QImage* outImage)
{
    // Stub: Particle rasterization delegated to ParticleView wrapper
}

std::vector<RenderParticle> ParticleRasterizer::applyCulling(
    const ParticleView* view, const std::vector<RenderParticle>& particles)
{
    return particles;
}

std::vector<RenderParticle> ParticleRasterizer::applyLodFilter(
    const std::vector<RenderParticle>& particles, int targetCount)
{
    return particles;
}

}  // namespace bltzr_qt::particleView
