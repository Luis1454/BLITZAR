#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_THROUGHPUTADVISOR_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_THROUGHPUTADVISOR_HPP_
#include <cstdint>
#include <string>
struct SimulationConfig;
namespace grav_qt {
enum class ThroughputAdvisorySeverity { None, Advisory, Warning };
struct ThroughputAdvisory final {
    ThroughputAdvisorySeverity severity = ThroughputAdvisorySeverity::None;
    std::uint32_t estimatedSubsteps = 1u;
    float estimatedStepsPerSecond = 0.0f;
    std::string summary;
    std::string action;
    std::string statusBarText;
};
class ThroughputAdvisor final {
public:
    static ThroughputAdvisory evaluate(const SimulationConfig& config, std::uint32_t drawCap);
};
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_THROUGHPUTADVISOR_HPP_
