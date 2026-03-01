#ifndef GRAVITY_UI_PARTICLEVIEWCOLOR_H
#define GRAVITY_UI_PARTICLEVIEWCOLOR_H

#include "ui/QtViewMath.hpp"

#include <QtGlobal>

#include <vector>

namespace grav_qt {

void updateAdaptiveScales(
    const std::vector<RenderParticle> &snapshot,
    float &adaptiveTemperatureScale,
    float &adaptivePressureScale
);
QRgb particleRampColorFast(const RenderParticle &particle, float temperatureScale, float pressureScale, int luminosity);
QRgb heavyBodyColor(int luminosity);
bool isHeavyBody(const RenderParticle &particle);

} // namespace grav_qt

#endif // GRAVITY_UI_PARTICLEVIEWCOLOR_H
