/*
 * @file modules/qt/src/widgets/overlays/Octree.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef BLITZAR_MODULES_QT_SRC_WIDGETS_OVERLAYS_OCTREE_HPP_
#define BLITZAR_MODULES_QT_SRC_WIDGETS_OVERLAYS_OCTREE_HPP_
/*
 * Module: qt
 * Responsibility: Build a lightweight octree-cell overlay from the displayed
 * snapshot.
 */
#include "types/SimulationTypes.hpp"
#include <vector>

namespace bltzr_qt {
struct OverlayBounds final {
    float centerX;
    float centerY;
    float centerZ;
    float halfSize;
};

struct OctreeNode final {
    float centerX;
    float centerY;
    float centerZ;
    float halfSize;
    int depth;
};

class Octree {
public:
    static std::vector<OctreeNode> build(const std::vector<RenderParticle>& particles,
                                                int maxDepth);
};
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_SRC_WIDGETS_OVERLAYS_OCTREE_HPP_
