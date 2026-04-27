/*
 * @file engine/include/config/SimulationProfile.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONPROFILE_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONPROFILE_HPP_
#include <string>
#include <string_view>
/*
 * @brief Defines the simulation config type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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
