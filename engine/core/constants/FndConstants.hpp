/*
 * @file engine/core/constants/FndConstants.hpp
 * Centralized cross-module constants for BLITZAR.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONSTANTS_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONSTANTS_HPP_

#include <algorithm>
#include <cmath>
#include <cstdint>

// Simulation time-step limits
static constexpr float kMinSimulationDt = 1.0e-6f;
static constexpr float kUiSimulationDtMin = 1.0e-5f;
static constexpr float kMaxStableInteractiveDt = 10.0f;
static constexpr float kDefaultSimulationDt = 0.01f;
static constexpr float kGalaxyCollisionDt = 0.02f;

// Physics defaults
static constexpr float kPhysicsMaxAccelerationDefault = 64.0f;
static constexpr float kPhysicsMinSofteningDefault = 1.0e-4f;
static constexpr float kPhysicsMinDistance2Default = 1.0e-12f;
static constexpr float kPhysicsMinTheta = 0.05f;
static constexpr float kPhysicsMaxTheta = 4.0f;
static constexpr float kOctreeSofteningMax = 5.0f;
static constexpr float kSphMaxAccelerationDefault = 40.0f;
static constexpr float kSphMaxSpeedDefault = 120.0f;
static constexpr float kSphSmoothingMin = 0.05f;
static constexpr float kSphSmoothingMax = 10.0f;
static constexpr float kSphRestDensityMin = 0.05f;
static constexpr float kSphRestDensityMax = 1000.0f;
static constexpr float kSphGasConstantMin = 0.01f;
static constexpr float kSphGasConstantMax = 1000.0f;
static constexpr float kSphViscosityMin = 0.0f;
static constexpr float kSphViscosityMax = 100.0f;
static constexpr float kHostFallbackPhysicsMaxAccelerationDefault = 1000.0f;
static constexpr float kHostFallbackPhysicsMinDistance2Default = 1.0e-8f;
static constexpr float kHostFallbackPhysicsMinThetaDefault = 0.1f;

// Mathematical constants
static constexpr float kPi = 3.14159265358979323846f;
static constexpr float kTwoPi = 2.0f * kPi;
static constexpr float kDegreesToRadians = kPi / 180.0f;

// Networking\runtime defaults
inline constexpr const char* kDefaultLoopbackHost = "127.0.0.1";
inline constexpr std::uint16_t kDefaultServerPort = 4545u;
inline constexpr std::uint16_t kNetworkPortMin = 1u;
inline constexpr std::uint16_t kNetworkPortMax = 65535u;
inline constexpr std::uint32_t kRuntimeRemoteTimeoutMinMs = 10u;
inline constexpr std::uint32_t kRuntimeRemoteTimeoutMaxMs = 60000u;
inline constexpr std::uint32_t kRuntimeRemoteCommandTimeoutDefaultMs = 80u;
inline constexpr std::uint32_t kRuntimeRemoteStatusTimeoutDefaultMs = 40u;
inline constexpr std::uint32_t kRuntimeRemoteSnapshotTimeoutDefaultMs = 140u;
inline constexpr int kServicePollIntervalMs = 200;
inline constexpr int kDaemonClientSocketTimeoutMs = 500;
inline constexpr int kQtStartupPollAttempts = 200;
inline constexpr int kQtStartupPollIntervalMs = 10;

// Rendering / UI
inline constexpr int kLuminosityMin = 0;
inline constexpr int kLuminosityMax = 255;
inline constexpr int kDefaultLuminosity = 100;
inline constexpr int kZoomSliderMin = 0;
inline constexpr int kZoomSliderMax = 400;
inline constexpr float kDefaultZoom = 8.0f;
inline constexpr float kViewportMinZoom = 0.0f;
inline constexpr float kRenderLODNearDistance = 10.0f;
inline constexpr float kRenderLODFarDistance = 60.0f;
inline constexpr float kZoomLogMin = 1.0e-6f;
inline constexpr float kZoomLogMax = 1.0e6f;
inline constexpr float kZoomCompensationMax = 24.0f;

inline float zoomCompensationLambda(float zoom)
{
    if (zoom <= kZoomLogMin)
        return kZoomCompensationMax;
    return std::clamp(kDefaultZoom / zoom, 1.0f, kZoomCompensationMax);
}

inline float zoomFromSliderValue(int value)
{
    const float normalized = static_cast<float>(std::clamp(value, kZoomSliderMin,
                                                            kZoomSliderMax) -
                                                 kZoomSliderMin) /
                             static_cast<float>(kZoomSliderMax - kZoomSliderMin);
    return kZoomLogMin * std::pow(kZoomLogMax / kZoomLogMin, normalized);
}

inline int zoomToSliderValue(float zoom)
{
    const float safeZoom = std::clamp(zoom, kZoomLogMin, kZoomLogMax);
    const float normalized = std::log(safeZoom / kZoomLogMin) /
                             std::log(kZoomLogMax / kZoomLogMin);
    return std::clamp(static_cast<int>(std::lround(
                           normalized * static_cast<float>(kZoomSliderMax - kZoomSliderMin))) +
                          kZoomSliderMin,
                      kZoomSliderMin, kZoomSliderMax);
}
inline constexpr int kOverlayOpacityDefault = 96;
inline constexpr int kOverlayDepthDefault = 3;
inline constexpr int kOverlayDepthMax = 8;

#endif // BLITZAR_ENGINE_INCLUDE_CONSTANTS_HPP_
