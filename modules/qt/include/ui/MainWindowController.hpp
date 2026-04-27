/*
 * @file modules/qt/include/ui/MainWindowController.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWCONTROLLER_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWCONTROLLER_HPP_
/*
 * Module: ui
 * Responsibility: Apply validated UI configuration changes to the client runtime.
 */
#include "client/IClientRuntime.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include <cstdint>
/*
 * @brief Defines the simulation config type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationConfig;

namespace grav_qt {
struct MainWindowApplyConfigResult final {
    grav_config::ScenarioValidationReport report;
    std::uint32_t clientDrawCap = 0u;
    bool applied = false;
};

class MainWindowController final {
public:
    MainWindowApplyConfigResult applyConfig(const SimulationConfig& config,
                                            grav_client::IClientRuntime& runtime,
                                            bool requestReset) const;
    std::uint32_t applyPerformanceProfile(const SimulationConfig& config,
                                          grav_client::IClientRuntime& runtime) const;
    grav_config::ScenarioValidationReport validate(const SimulationConfig& config) const;
};
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWCONTROLLER_HPP_
