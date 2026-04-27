// File: modules/qt/ui/ParticleViewColor.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "ui/ParticleViewColor.hpp"
namespace grav_qt {
QRgb toQRgb(const grav::ColorRGBA& c)
{
    return qRgba(c.r, c.g, c.b, c.a);
}
void updateAdaptiveScales(const std::vector<RenderParticle>& snapshot,
                          float& adaptiveTemperatureScale, float& adaptivePressureScale)
{
    grav::updateAdaptiveScales(snapshot, adaptiveTemperatureScale, adaptivePressureScale);
}
QRgb particleRampColorFast(const RenderParticle& particle, float temperatureScale,
                           float pressureScale, int luminosity)
{
    return toQRgb(
        grav::particleRampColorFast(particle, temperatureScale, pressureScale, luminosity));
}
QRgb heavyBodyColor(int luminosity)
{
    return toQRgb(grav::heavyBodyColor(luminosity));
}
bool isHeavyBody(const RenderParticle& particle)
{
    return grav::isHeavyBody(particle);
}
} // namespace grav_qt
