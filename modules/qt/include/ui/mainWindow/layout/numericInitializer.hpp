/*
 * @file modules/qt/include/ui/mainWindow/layout/numericInitializer.hpp
 * @brief Numeric control initialization utilities.
 */

#ifndef BLTZAR_QT_MAINWINDOW_LAYOUT_NUMERICINITIALIZER_HPP
#define BLTZAR_QT_MAINWINDOW_LAYOUT_NUMERICINITIALIZER_HPP

class MainWindow;

namespace bltzr_qt::mainWindow::layout {

/**
 * @brief Initialize spin boxes and sliders with ranges and initial values.
 */
class NumericInitializer final {
public:
    /**
     * @brief Set ranges, steps, and initial values for all numeric controls.
     * @param mainWindow Target main window with numeric controls
     */
    static void initializeSpinAndSliderValues(MainWindow* mainWindow);
};

}  // namespace bltzr_qt::mainWindow::layout

#endif  // BLTZAR_QT_MAINWINDOW_LAYOUT_NUMERICINITIALIZER_HPP
