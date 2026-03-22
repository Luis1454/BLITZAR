#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_OCTREEOVERLAY_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_OCTREEOVERLAY_HPP_

/*
 * Module: ui
 * Responsibility: Build a lightweight octree-cell overlay from the displayed snapshot.
 */

#include "types/SimulationTypes.hpp"

#include <vector>

namespace grav_qt {

struct OctreeOverlayNode final {
    float centerX;
    float centerY;
    float centerZ;
    float halfSize;
    int depth;
};

class OctreeOverlay {
    public:
        static std::vector<OctreeOverlayNode> build(const std::vector<RenderParticle> &particles, int maxDepth);
};

} // namespace grav_qt

#endif // GRAVITY_MODULES_QT_INCLUDE_UI_OCTREEOVERLAY_HPP_
