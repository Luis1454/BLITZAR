#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWPRESENTER_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWPRESENTER_HPP_

/*
 * Module: ui
 * Responsibility: Format runtime telemetry into user-facing status strings for the Qt workspace.
 */

#include "client/IClientRuntime.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

struct SimulationStats;

namespace grav_qt {

/// Collects the runtime fields required to render the workspace status area.
struct MainWindowPresentationInput final {
    SimulationStats stats;
    grav_client::SnapshotPipelineState snapshotPipeline;
    std::string linkLabel;
    std::string ownerLabel;
    std::string performanceProfile;
    std::size_t displayedParticles = 0u;
    std::uint32_t clientDrawCap = 0u;
    std::uint32_t statsAgeMs = 0u;
    std::uint32_t snapshotAgeMs = 0u;
    std::uint32_t snapshotLatencyMs = 0u;
    float uiTickFps = 0.0f;
    float simulationHorizonSeconds = 0.0f;
};

/// Holds the formatted strings consumed by status and trace widgets.
struct MainWindowPresentation final {
    std::string headlineText;
    std::string runtimeText;
    std::string queueText;
    std::string energyText;
    std::string gpuText;
    std::string statusText;
    std::string consoleTrace;
};

/// Formats runtime telemetry into stable Qt presentation strings.
class MainWindowPresenter final {
    public:
        /// Produces the status and trace strings shown by the main window.
        MainWindowPresentation present(const MainWindowPresentationInput &input) const;
};

} // namespace grav_qt

#endif // GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWPRESENTER_HPP_
