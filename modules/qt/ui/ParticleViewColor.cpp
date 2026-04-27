// File: modules/qt/ui/ParticleViewColor.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "ui/ParticleViewColor.hpp"
namespace grav_qt {
/// Description: Executes the toQRgb operation.
QRgb toQRgb(const grav::ColorRGBA& c)
{
    return qRgba(c.r, c.g, c.b, c.a);
}
void updateAdaptiveScales(const std::vector<RenderParticle>& snapshot,
                          float& adaptiveTemperatureScale, float& adaptivePressureScale)
{
    /// Description: Executes the updateAdaptiveScales operation.
    grav::updateAdaptiveScales(snapshot, adaptiveTemperatureScale, adaptivePressureScale);
}
QRgb particleRampColorFast(const RenderParticle& particle, float temperatureScale,
                           float pressureScale, int luminosity)
{
    return toQRgb(
        /// Description: Executes the particleRampColorFast operation.
        grav::particleRampColorFast(particle, temperatureScale, pressureScale, luminosity));
}
/// Description: Executes the heavyBodyColor operation.
QRgb heavyBodyColor(int luminosity)
{
    return toQRgb(grav::heavyBodyColor(luminosity));
}
/// Description: Executes the isHeavyBody operation.
bool isHeavyBody(const RenderParticle& particle)
{
    return grav::isHeavyBody(particle);
}
} // namespace grav_qt
