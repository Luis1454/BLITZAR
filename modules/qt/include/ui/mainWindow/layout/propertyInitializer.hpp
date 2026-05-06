/*
 * @file modules/qt/include/ui/mainWindow/layout/propertyInitializer.hpp
 * @brief Property and tooltip initialization utilities.
 */

#ifndef BLTZAR_QT_MAINWINDOW_LAYOUT_PROPERTYINITIALIZER_HPP
#define BLTZAR_QT_MAINWINDOW_LAYOUT_PROPERTYINITIALIZER_HPP

class MainWindow;

namespace bltzr_qt::mainWindow::layout {

/**
 * @brief Initialize object names, tooltips, and UI properties.
 */
class PropertyInitializer final {
public:
    /**
     * @brief Set object names for testing and accessibility.
     * @param mainWindow Target main window
     */
    static void initializeObjectNames(MainWindow* mainWindow);

    /**
     * @brief Set tooltips, labels, and text properties.
     * @param mainWindow Target main window
     */
    static void initializeLabelsAndTooltips(MainWindow* mainWindow);
};

}  // namespace bltzr_qt::mainWindow::layout

#endif  // BLTZAR_QT_MAINWINDOW_LAYOUT_PROPERTYINITIALIZER_HPP
