/*
 * @file modules/qt/include/ui/MainWindowPresenter.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWPRESENTER_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWPRESENTER_HPP_
/*
 * Module: ui
 * Responsibility: Format runtime telemetry into user-facing status strings for the
 * Qt workspace.
 */
#include "client/IClientRuntime.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
/*
 * @brief Defines the simulation stats type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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
    float simulationHorizonSeconds = 0.0f;
};

struct MainWindowPresentation final {
    std::string headlineText;
    std::string runtimeText;
    std::string queueText;
    std::string energyText;
    std::string gpuText;
    std::string statusText;
    std::string consoleTrace;
};

class MainWindowPresenter final {
public:
    MainWindowPresentation present(const MainWindowPresentationInput& input) const;
};
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOWPRESENTER_HPP_
