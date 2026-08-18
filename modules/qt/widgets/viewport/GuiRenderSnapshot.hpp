/*
 * @file modules/qt/widgets/viewport/GuiRenderSnapshot.hpp
 * @brief Prepare simulation snapshots for viewport rendering.
 */

#ifndef BLITZAR_MODULES_QT_SRC_WIDGETS_VIEWPORT_RENDERSNAPSHOT_HPP_
#define BLITZAR_MODULES_QT_SRC_WIDGETS_VIEWPORT_RENDERSNAPSHOT_HPP_

#include "widgets/viewport/GuiParticle.hpp"
#include <cstddef>
#include <vector>

namespace bltzr_qt {
std::vector<RenderParticle> prepareRenderSnapshot(std::vector<RenderParticle> snapshot,
                                                  std::size_t cap);
} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_SRC_WIDGETS_VIEWPORT_RENDERSNAPSHOT_HPP_
