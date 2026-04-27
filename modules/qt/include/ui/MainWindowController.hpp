// File: modules/qt/include/ui/MainWindowController.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWCONTROLLER_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWCONTROLLER_HPP_
/*
 * Module: ui
 * Responsibility: Apply validated UI configuration changes to the client runtime.
 */
#include "client/IClientRuntime.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include <cstdint>
/// Description: Defines the SimulationConfig data or behavior contract.
struct SimulationConfig;
namespace grav_qt {
/// Reports whether a configuration update was validated and applied to the runtime.
struct MainWindowApplyConfigResult final {
    grav_config::ScenarioValidationReport report;
    std::uint32_t clientDrawCap = 0u;
    bool applied = false;
};
/// Bridges Qt configuration edits to the runtime and validation layers.
class MainWindowController final {
public:
    /// Validates `config`, applies it to `runtime`, and optionally requests a reset.
    MainWindowApplyConfigResult applyConfig(const SimulationConfig& config,
                                            grav_client::IClientRuntime& runtime,
                                            bool requestReset) const;
    /// Applies the performance profile subset and returns the resulting draw cap.
    std::uint32_t applyPerformanceProfile(const SimulationConfig& config,
                                          grav_client::IClientRuntime& runtime) const;
    /// Validates a configuration without mutating runtime state.
    grav_config::ScenarioValidationReport validate(const SimulationConfig& config) const;
};
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWCONTROLLER_HPP_
