// File: modules/qt/include/ui/ThroughputAdvisor.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_THROUGHPUTADVISOR_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_THROUGHPUTADVISOR_HPP_
#include <cstdint>
#include <string>
/// Description: Defines the SimulationConfig data or behavior contract.
struct SimulationConfig;

namespace grav_qt {
/// Description: Enumerates the supported ThroughputAdvisorySeverity values.
enum class ThroughputAdvisorySeverity {
    None,
    Advisory,
    Warning
};

/// Description: Defines the ThroughputAdvisory data or behavior contract.
struct ThroughputAdvisory final {
    ThroughputAdvisorySeverity severity = ThroughputAdvisorySeverity::None;
    std::uint32_t estimatedSubsteps = 1u;
    float estimatedStepsPerSecond = 0.0f;
    std::string summary;
    std::string action;
    std::string statusBarText;
};

/// Description: Defines the ThroughputAdvisor data or behavior contract.
class ThroughputAdvisor final {
public:
    /// Description: Describes the evaluate operation contract.
    static ThroughputAdvisory evaluate(const SimulationConfig& config, std::uint32_t drawCap);
};
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_THROUGHPUTADVISOR_HPP_
