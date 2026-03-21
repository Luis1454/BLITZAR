#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWPRESENTER_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWPRESENTER_HPP_

#include "client/IClientRuntime.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

struct SimulationStats;

namespace grav_qt {

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
};

struct MainWindowPresentation final {
    std::string headlineText;
    std::string runtimeText;
    std::string queueText;
    std::string energyText;
    std::string statusText;
    std::string consoleTrace;
};

class MainWindowPresenter final {
    public:
        MainWindowPresentation present(const MainWindowPresentationInput &input) const;
};

} // namespace grav_qt

#endif // GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWPRESENTER_HPP_
