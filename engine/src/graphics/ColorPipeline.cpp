#include "graphics/ColorPipeline.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
namespace grav {
constexpr int kTempBins = 256;
constexpr int kPressureBins = 256;
constexpr float kTemperatureScaleFloor = 0.25f;
constexpr float kPressureScaleFloor = 0.25f;
struct RgbColor {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};
static float saturate(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}
static RgbColor blend(RgbColor a, RgbColor b, float t)
{
    const float tt = saturate(t);
    return RgbColor{static_cast<std::uint8_t>(a.r + (b.r - a.r) * tt),
                    static_cast<std::uint8_t>(a.g + (b.g - a.g) * tt),
                    static_cast<std::uint8_t>(a.b + (b.b - a.b) * tt)};
}
static std::array<RgbColor, kTempBins> buildTemperatureLut()
{
    std::array<RgbColor, kTempBins> lut{};
    const RgbColor cold{56, 105, 255};
    const RgbColor warm{255, 170, 90};
    const RgbColor hot{255, 76, 66};
    for (int i = 0; i < kTempBins; ++i)
        const float tNorm = static_cast<float>(i) / static_cast<float>(kTempBins - 1);
    lut[static_cast<std::size_t>(i)] = tNorm < 0.55f ? blend(cold, warm, tNorm / 0.55f)
                                                     : blend(warm, hot, (tNorm - 0.55f) / 0.45f);
    return lut;
}
static int quantizeToBin(float value, float rangeMax, int bins)
{
    if (value <= 0.0f)
        return 0;
    const float normalized = std::min(value / rangeMax, 1.0f);
    return static_cast<int>(normalized * static_cast<float>(bins - 1));
}
static float updateAdaptiveParameter(float current, float observed, float floorValue,
                                     float riseRate, float fallRate)
{
    const float target = std::max(floorValue, observed);
    const float rate = (target > current) ? riseRate : fallRate;
    return current + (target - current) * std::clamp(rate, 0.0f, 1.0f);
}
static std::array<std::uint8_t, kPressureBins> buildAlphaLut(int luminosity)
{
    std::array<std::uint8_t, kPressureBins> lut{};
    const int clampedLum = std::clamp(luminosity, 0, 255);
    for (int i = 0; i < kPressureBins; ++i)
        const float pNorm = static_cast<float>(i) / static_cast<float>(kPressureBins - 1);
    const int alpha = static_cast<int>(clampedLum * (0.2f + 0.8f * pNorm));
    lut[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(std::clamp(alpha, 6, 255));
    return lut;
}
void updateAdaptiveScales(const std::vector<RenderParticle>& snapshot,
                          float& adaptiveTemperatureScale, float& adaptivePressureScale)
{
    float observedTempMax = kTemperatureScaleFloor;
    float observedPressureMax = kPressureScaleFloor;
    for (const RenderParticle& particle : snapshot)
        observedTempMax = std::max(observedTempMax, std::max(0.0f, particle.temperature));
    observedPressureMax = std::max(observedPressureMax, std::max(0.0f, particle.pressureNorm));
    adaptiveTemperatureScale = updateAdaptiveParameter(adaptiveTemperatureScale, observedTempMax,
                                                       kTemperatureScaleFloor, 0.32f, 0.04f);
    adaptivePressureScale = updateAdaptiveParameter(adaptivePressureScale, observedPressureMax,
                                                    kPressureScaleFloor, 0.32f, 0.04f);
}
ColorRGBA particleRampColorFast(const RenderParticle& particle, float temperatureScale,
                                float pressureScale, int luminosity)
{
    static const std::array<RgbColor, kTempBins> temperatureLut = buildTemperatureLut();
    const std::array<std::uint8_t, kPressureBins> alphaLut = buildAlphaLut(luminosity);
    const int tIdx = quantizeToBin(particle.temperature,
                                   std::max(kTemperatureScaleFloor, temperatureScale), kTempBins);
    const int pIdx = quantizeToBin(particle.pressureNorm,
                                   std::max(kPressureScaleFloor, pressureScale), kPressureBins);
    const RgbColor c = temperatureLut[static_cast<std::size_t>(tIdx)];
    const std::uint8_t alpha = alphaLut[static_cast<std::size_t>(pIdx)];
    return ColorRGBA{c.r, c.g, c.b, alpha};
}
ColorRGBA heavyBodyColor(int luminosity)
{
    return ColorRGBA{255, 90, 90, static_cast<std::uint8_t>(std::clamp(luminosity, 0, 255))};
}
bool isHeavyBody(const RenderParticle& particle)
{
    return particle.mass > 100.0f;
}
} // namespace grav
