// File: engine/include/graphics/ColorPipeline.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_GRAPHICS_COLORPIPELINE_HPP_
#define GRAVITY_ENGINE_INCLUDE_GRAPHICS_COLORPIPELINE_HPP_
#include "graphics/GraphicsTypes.hpp"
#include "types/SimulationTypes.hpp"
#include <vector>
namespace grav {
void updateAdaptiveScales(const std::vector<RenderParticle>& snapshot,
                          float& adaptiveTemperatureScale, float& adaptivePressureScale);
ColorRGBA particleRampColorFast(const RenderParticle& particle, float temperatureScale,
                                float pressureScale, int luminosity);
/// Description: Executes the heavyBodyColor operation.
ColorRGBA heavyBodyColor(int luminosity);
/// Description: Executes the isHeavyBody operation.
bool isHeavyBody(const RenderParticle& particle);
} // namespace grav
#endif // GRAVITY_ENGINE_INCLUDE_GRAPHICS_COLORPIPELINE_HPP_
