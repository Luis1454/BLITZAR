#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONPERFORMANCEPROFILE_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONPERFORMANCEPROFILE_HPP_
#include <string>
#include <string_view>
struct SimulationConfig;
namespace grav_config {
inline constexpr std::string_view kPerformanceProfileInteractive = "interactive";
inline constexpr std::string_view kPerformanceProfileBalanced = "balanced";
inline constexpr std::string_view kPerformanceProfileQuality = "quality";
inline constexpr std::string_view kPerformanceProfileCustom = "custom";
[[nodiscard]] bool normalizePerformanceProfile(std::string_view raw, std::string& outCanonical);
void applyPerformanceProfile(SimulationConfig& config);
[[nodiscard]] bool isPerformanceManagedField(std::string_view key);
} // namespace grav_config
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_SIMULATIONPERFORMANCEPROFILE_HPP_
