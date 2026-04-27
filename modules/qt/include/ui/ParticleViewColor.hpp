/*
 * @file modules/qt/include/ui/ParticleViewColor.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEWCOLOR_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEWCOLOR_HPP_
/*
 * Module: ui
 * Responsibility: Convert simulation attributes into Qt-friendly particle colors.
 */
#include "graphics/ColorPipeline.hpp"
#include <QColor>
#include <QtGlobal>
#include <vector>

namespace grav_qt {
QRgb toQRgb(const grav::ColorRGBA& c);
void updateAdaptiveScales(const std::vector<RenderParticle>& snapshot,
                          float& adaptiveTemperatureScale, float& adaptivePressureScale);
QRgb particleRampColorFast(const RenderParticle& particle, float temperatureScale,
                           float pressureScale, int luminosity);
QRgb heavyBodyColor(int luminosity);
bool isHeavyBody(const RenderParticle& particle);
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEWCOLOR_HPP_
