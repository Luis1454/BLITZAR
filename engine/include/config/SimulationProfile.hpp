// File: engine/include/config/SimulationProfile.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONPROFILE_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONPROFILE_HPP_
#include <string>
#include <string_view>
struct SimulationConfig;
namespace grav_config {
inline constexpr std::string_view kSimulationProfileDiskOrbit = "disk_orbit";
inline constexpr std::string_view kSimulationProfileGalaxyCollision = "galaxy_collision";
inline constexpr std::string_view kSimulationProfilePlummerSphere = "plummer_sphere";
inline constexpr std::string_view kSimulationProfileBinaryStar = "binary_star";
inline constexpr std::string_view kSimulationProfileSolarSystem = "solar_system";
inline constexpr std::string_view kSimulationProfileSphCollapse = "sph_collapse";
[[nodiscard]] bool normalizeSimulationProfile(std::string_view raw, std::string& outCanonical);
void applySimulationProfile(SimulationConfig& config);
} // namespace grav_config
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONPROFILE_HPP_
