#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWCONTROLLER_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWCONTROLLER_HPP_

#include "client/IClientRuntime.hpp"
#include "config/SimulationScenarioValidation.hpp"

#include <cstdint>

struct SimulationConfig;

namespace grav_qt {

struct MainWindowApplyConfigResult final {
    grav_config::ScenarioValidationReport report;
    std::uint32_t clientDrawCap = 0u;
    bool applied = false;
};

class MainWindowController final {
    public:
        MainWindowApplyConfigResult applyConfig(
            const SimulationConfig &config,
            grav_client::IClientRuntime &runtime,
            bool requestReset) const;
        std::uint32_t applyPerformanceProfile(
            const SimulationConfig &config,
            grav_client::IClientRuntime &runtime) const;
        grav_config::ScenarioValidationReport validate(const SimulationConfig &config) const;
};

} // namespace grav_qt

#endif // GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWCONTROLLER_HPP_
