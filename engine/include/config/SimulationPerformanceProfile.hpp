// File: engine/include/config/SimulationPerformanceProfile.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONPERFORMANCEPROFILE_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONPERFORMANCEPROFILE_HPP_
#include <string>
#include <string_view>
/// Description: Defines the SimulationConfig data or behavior contract.
struct SimulationConfig;

namespace grav_config {
inline constexpr std::string_view kPerformanceProfileInteractive = "interactive";
inline constexpr std::string_view kPerformanceProfileBalanced = "balanced";
inline constexpr std::string_view kPerformanceProfileQuality = "quality";
inline constexpr std::string_view kPerformanceProfileCustom = "custom";
/// Description: Describes the normalize performance profile operation contract.
[[nodiscard]] bool normalizePerformanceProfile(std::string_view raw, std::string& outCanonical);
/// Description: Executes the applyPerformanceProfile operation.
void applyPerformanceProfile(SimulationConfig& config);
/// Description: Describes the is performance managed field operation contract.
[[nodiscard]] bool isPerformanceManagedField(std::string_view key);
} // namespace grav_config
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONPERFORMANCEPROFILE_HPP_
