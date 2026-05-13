/*
 * @file modules/qt/src/support/performance/Throughput.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef BLITZAR_MODULES_QT_SRC_SUPPORT_PERFORMANCE_THROUGHPUT_HPP_
#define BLITZAR_MODULES_QT_SRC_SUPPORT_PERFORMANCE_THROUGHPUT_HPP_
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
enum class Severity {
    None,
    Advisory,
    Warning
};

struct Advisory final {
    Severity severity = Severity::None;
    std::uint32_t estimatedSubsteps = 1u;
    float estimatedStepsPerSecond = 0.0f;
    std::string summary;
    std::string action;
    std::string statusBarText;
};

class ThroughputLocal final {
public:
    static std::uint32_t estimateSubsteps(const SimulationConfig& config);
    static float solverPenalty(const SimulationConfig& config, std::uint32_t substeps);
    static float drawPenalty(const SimulationConfig& config, std::uint32_t drawCap);
    static std::string suggestedAction(const SimulationConfig& config, std::uint32_t substeps,
                                       std::uint32_t drawCap);
};

class Throughput final {
public:
    static Advisory evaluate(const SimulationConfig& config, std::uint32_t drawCap);
};
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_SRC_SUPPORT_PERFORMANCE_THROUGHPUT_HPP_
