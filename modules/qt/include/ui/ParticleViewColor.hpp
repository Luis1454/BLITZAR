#pragma once

#include "ui/QtViewMath.hpp"

#include <QtGlobal>
#include <QColor>

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

