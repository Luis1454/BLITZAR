#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEWCOLOR_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEWCOLOR_HPP_

#include "graphics/ColorPipeline.hpp"

#include <QColor>
#include <QtGlobal>

#include <vector>

namespace grav_qt {

// Bridge generic ColorRGBA to Qt QRgb
inline QRgb toQRgb(const grav::ColorRGBA &c)
{
    return qRgba(c.r, c.g, c.b, c.a);
}

void updateAdaptiveScales(
    const std::vector<RenderParticle> &snapshot,
    float &adaptiveTemperatureScale,
    float &adaptivePressureScale
);

QRgb particleRampColorFast(const RenderParticle &particle, float temperatureScale, float pressureScale, int luminosity);
QRgb heavyBodyColor(int luminosity);
bool isHeavyBody(const RenderParticle &particle);

} // namespace grav_qt


#endif // GRAVITY_MODULES_QT_INCLUDE_UI_PARTICLEVIEWCOLOR_HPP_
