/*
 * @file modules/qt/include/ui/mainWindow/presenter/stateComputers.hpp
 * @brief State derivation helpers.
 */

#ifndef BLTZAR_QT_MAINWINDOW_PRESENTER_STATECOMPUTERS_HPP
#define BLTZAR_QT_MAINWINDOW_PRESENTER_STATECOMPUTERS_HPP

#include "ui/MainWindowPresenter.hpp"
#include <cstdint>
#include <string>

namespace bltzr_qt::mainWindow::presenter {

/**
 * @brief Compute derived state flags and display values.
 */
class StateComputers final {
public:
    /**
     * @brief Determine if telemetry age is stale (> 1000ms).
     * @param ageMs Age in milliseconds
     * @return True if stale
     */
    static bool isStale(std::uint32_t ageMs);

    /**
     * @brief Determine if age value is valid (not max uint32_t).
     * @param ageMs Age value
     * @return True if valid
     */
    static bool hasAge(std::uint32_t ageMs);

    /**
     * @brief Compute backend state label.
     * @param input Presentation input
     * @return State string
     */
    static std::string backendStateLabel(const MainWindowPresentationInput& input);

    /**
     * @brief Compute viewport state label.
     * @param input Presentation input
     * @return State string
     */
    static std::string viewportStateLabel(const MainWindowPresentationInput& input);

    /**
     * @brief Compute progress percentage label.
     * @param input Presentation input
     * @return Progress string
     */
    static std::string progressLabel(const MainWindowPresentationInput& input);

    /**
     * @brief Compute ETA label in seconds.
     * @param input Presentation input
     * @return ETA string
     */
    static std::string etaLabel(const MainWindowPresentationInput& input);

    /**
     * @brief Compute export state label.
     * @param input Presentation input
     * @return Export state string
     */
    static std::string exportStateLabel(const MainWindowPresentationInput& input);

    /**
     * @brief Compute simulated seconds per real second.
     * @param input Presentation input
     * @return Throughput ratio
     */
    static float simulatedSecondsPerSecond(const MainWindowPresentationInput& input);
};

}  // namespace bltzr_qt::mainWindow::presenter

#endif  // BLTZAR_QT_MAINWINDOW_PRESENTER_STATECOMPUTERS_HPP
