#include "ui/ParticleViewColor.hpp"

namespace grav_qt {

void updateAdaptiveScales(
    const std::vector<RenderParticle> &snapshot,
    float &adaptiveTemperatureScale,
    float &adaptivePressureScale
)
{
    grav::updateAdaptiveScales(snapshot, adaptiveTemperatureScale, adaptivePressureScale);
}

QRgb particleRampColorFast(const RenderParticle &particle, float temperatureScale, float pressureScale, int luminosity)
{
    return toQRgb(grav::particleRampColorFast(particle, temperatureScale, pressureScale, luminosity));
}

QRgb heavyBodyColor(int luminosity)
{
    return toQRgb(grav::heavyBodyColor(luminosity));
}

bool isHeavyBody(const RenderParticle &particle)
{
    return grav::isHeavyBody(particle);
}

} // namespace grav_qt
