#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEWCOLOR_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEWCOLOR_HPP_
/* * Module: ui * Responsibility: Convert simulation attributes into Qt-friendly particle colors.
 */
#include "graphics/ColorPipeline.hpp"
#include <QColor>
#include <QtGlobal>
#include <vector>
namespace grav_qt {
/// Converts a generic RGBA color into the packed Qt pixel representation.
QRgb toQRgb(const grav::ColorRGBA& c);
/// Recomputes adaptive temperature and pressure scales for the current snapshot.
void updateAdaptiveScales(const std::vector<RenderParticle>& snapshot,
                          float& adaptiveTemperatureScale, float& adaptivePressureScale);
/// Maps a render particle to a fast ramp color using the provided adaptive scales.
QRgb particleRampColorFast(const RenderParticle& particle, float temperatureScale,
                           float pressureScale, int luminosity);
/// Returns the color used for heavy-body markers such as central masses.
QRgb heavyBodyColor(int luminosity);
/// Reports whether the particle should be highlighted as a heavy body.
bool isHeavyBody(const RenderParticle& particle);
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEWCOLOR_HPP_
