/*
 * @file modules/qt/include/ui/mainWindow/presenter/telemetryAggregator.hpp
 * @brief Telemetry presentation aggregation.
 */

#ifndef BLTZAR_QT_MAINWINDOW_PRESENTER_TELEMETRYAGGREGATOR_HPP
#define BLTZAR_QT_MAINWINDOW_PRESENTER_TELEMETRYAGGREGATOR_HPP

#include "ui/MainWindowPresenter.hpp"
#include <string>

namespace bltzr_qt::mainWindow::presenter {

/**
 * @brief Aggregate presentation input into display sections.
 */
class TelemetryAggregator final {
public:
    /**
     * @brief Build headline text (simulation time, step count).
     * @param input Presentation input
     * @return Formatted headline
     */
    static std::string buildHeadline(const MainWindowPresentationInput& input);

    /**
     * @brief Build runtime telemetry section.
     * @param input Presentation input
     * @return Formatted runtime section
     */
    static std::string buildRuntimeSection(const MainWindowPresentationInput& input);

    /**
     * @brief Build queue depth section.
     * @param input Presentation input
     * @return Formatted queue section
     */
    static std::string buildQueueSection(const MainWindowPresentationInput& input);

    /**
     * @brief Build energy breakdown section.
     * @param input Presentation input
     * @return Formatted energy section
     */
    static std::string buildEnergySection(const MainWindowPresentationInput& input);

    /**
     * @brief Build GPU resource section.
     * @param input Presentation input
     * @return Formatted GPU section
     */
    static std::string buildGpuSection(const MainWindowPresentationInput& input);

    /**
     * @brief Build simulation status section.
     * @param input Presentation input
     * @return Formatted status section
     */
    static std::string buildStatusSection(const MainWindowPresentationInput& input);

    /**
     * @brief Build console trace section.
     * @param input Presentation input
     * @return Formatted trace section
     */
    static std::string buildTraceSection(const MainWindowPresentationInput& input);
};

}  // namespace bltzr_qt::mainWindow::presenter

#endif  // BLTZAR_QT_MAINWINDOW_PRESENTER_TELEMETRYAGGREGATOR_HPP
