/*
 * @file modules/qt/include/ui/particleView/particleRasterizer.hpp
 * @brief Core particle rendering and rasterization.
 */

#ifndef BLTZAR_QT_PARTICLEVIEW_PARTICLERASTERIZER_HPP
#define BLTZAR_QT_PARTICLEVIEW_PARTICLERASTERIZER_HPP

#include <vector>

class QImage;
class ParticleView;
struct RenderParticle;

namespace bltzr_qt::particleView {

/**
 * @brief Rasterize particle data to framebuffer with LOD and color mapping.
 */
class ParticleRasterizer final {
public:
    /**
     * @brief Render particles to image with LOD filtering, projection, and color mapping.
     * @param view Source particle view with camera/zoom state
     * @param particles Particle snapshot to render
     * @param outImage Output image framebuffer
     */
    static void rasterizeParticles(const ParticleView* view,
                                   const std::vector<RenderParticle>& particles,
                                   QImage* outImage);

    /**
     * @brief Apply frustum culling to particle set.
     * @param view Source particle view
     * @param particles Input particles
     * @return Visible particles after culling
     */
    static std::vector<RenderParticle> applyCulling(const ParticleView* view,
                                                    const std::vector<RenderParticle>& particles);

    /**
     * @brief Apply LOD filtering to reduce rendered count on high loads.
     * @param particles Input particles
     * @param targetCount Target particle count
     * @return Filtered particles
     */
    static std::vector<RenderParticle> applyLodFilter(const std::vector<RenderParticle>& particles,
                                                      int targetCount);
};

}  // namespace bltzr_qt::particleView

#endif  // BLTZAR_QT_PARTICLEVIEW_PARTICLERASTERIZER_HPP
