/*
 * @file modules/qt/include/ui/OctreeOverlay.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_OCTREEOVERLAY_HPP_
#define BLITZAR_MODULES_QT_INCLUDE_UI_OCTREEOVERLAY_HPP_
/*
 * Module: ui
 * Responsibility: Build a lightweight octree-cell overlay from the displayed
 * snapshot.
 */
#include "types/SimulationTypes.hpp"
#include <vector>

namespace bltzr_qt {
struct OctreeOverlayNode final {
    float centerX;
    float centerY;
    float centerZ;
    float halfSize;
    int depth;
};

class OctreeOverlay {
public:
    static std::vector<OctreeOverlayNode> build(const std::vector<RenderParticle>& particles,
                                                int maxDepth);
};
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_INCLUDE_UI_OCTREEOVERLAY_HPP_
