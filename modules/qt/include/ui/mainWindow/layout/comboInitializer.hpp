/*
 * @file modules/qt/include/ui/mainWindow/layout/comboInitializer.hpp
 * @brief Combo box initialization utilities.
 */

#ifndef BLTZAR_QT_MAINWINDOW_LAYOUT_COMBOINITIALIZER_HPP
#define BLTZAR_QT_MAINWINDOW_LAYOUT_COMBOINITIALIZER_HPP

class MainWindow;

namespace bltzr_qt::mainWindow::layout {

/**
 * @brief Initialize all combo boxes with enumeration values.
 */
class ComboInitializer final {
public:
    /**
     * @brief Populate solver, integrator, performance, preset, and view mode combo boxes.
     * @param mainWindow Target main window with combo controls
     */
    static void initializeComboBoxes(MainWindow* mainWindow);
};

}  // namespace bltzr_qt::mainWindow::layout

#endif  // BLTZAR_QT_MAINWINDOW_LAYOUT_COMBOINITIALIZER_HPP
