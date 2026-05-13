/*
 * @file modules/qt/src/window/control/Controller.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef BLITZAR_MODULES_QT_SRC_WINDOW_CONTROL_CONTROLLER_HPP_
#define BLITZAR_MODULES_QT_SRC_WINDOW_CONTROL_CONTROLLER_HPP_
/*
 * Module: qt
 * Responsibility: Apply validated UI configuration changes to the client runtime.
 */
#include "client/runtime/Interface.hpp"
#include "config/validation/Scenario.hpp"
#include <cstdint>
/*
 * @brief Defines the simulation config type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationConfig;

namespace bltzr_qt {
struct ApplyConfigResult final {
    bltzr_config::ScenarioValidationReport report;
    std::uint32_t clientDrawCap = 0u;
    bool applied = false;
};

class Controller final {
public:
    ApplyConfigResult applyConfig(const SimulationConfig& config,
                                            bltzr_client::Interface& runtime,
                                            bool requestReset) const;
    std::uint32_t applyPerformanceProfile(const SimulationConfig& config,
                                          bltzr_client::Interface& runtime) const;
    bltzr_config::ScenarioValidationReport validate(const SimulationConfig& config) const;
};
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_SRC_WINDOW_CONTROL_CONTROLLER_HPP_
