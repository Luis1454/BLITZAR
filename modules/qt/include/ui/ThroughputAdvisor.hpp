/*
 * @file modules/qt/include/ui/ThroughputAdvisor.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_THROUGHPUTADVISOR_HPP_
#define BLITZAR_MODULES_QT_INCLUDE_UI_THROUGHPUTADVISOR_HPP_
#include <cstdint>
#include <string>
/*
 * @brief Defines the simulation config type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationConfig;

namespace bltzr_qt {
enum class ThroughputAdvisorySeverity {
    None,
    Advisory,
    Warning
};

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
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_INCLUDE_UI_THROUGHPUTADVISOR_HPP_
