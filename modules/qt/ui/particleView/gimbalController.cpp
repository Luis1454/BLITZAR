/*
 * @file modules/qt/ui/particleView/gimbalController.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Gimbal controller implementations.
 */

#include "ui/particleView/gimbalController.hpp"

namespace bltzr_qt::particleView {

void GimbalController::handleMousePress(ParticleView* view, QMouseEvent* event)
{
    // Stub: Mouse press handling delegated to ParticleView wrapper
}

void GimbalController::handleMouseMove(ParticleView* view, QMouseEvent* event)
{
    // Stub: Mouse move handling delegated to ParticleView wrapper
}

void GimbalController::handleMouseRelease(ParticleView* view, QMouseEvent* event)
{
    // Stub: Mouse release handling delegated to ParticleView wrapper
}

GimbalAxis GimbalController::pickAxis(ParticleView* view, int x, int y)
{
    // Stub: Axis picking delegated to ParticleView wrapper
    return GimbalAxis::None;
}

}  // namespace bltzr_qt::particleView
