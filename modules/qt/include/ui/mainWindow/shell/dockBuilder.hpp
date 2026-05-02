/*
 * @file modules/qt/include/ui/mainWindow/shell/dockBuilder.hpp
 * @brief Dock widget construction utilities.
 */

#ifndef BLTZAR_QT_MAINWINDOW_SHELL_DOCKBUILDER_HPP
#define BLTZAR_QT_MAINWINDOW_SHELL_DOCKBUILDER_HPP

class QTabWidget;
class QWidget;
class MainWindow;

namespace bltzr_qt::mainWindow::shell {

/**
 * @brief Build and configure dock widgets.
 */
class DockBuilder final {
public:
    /**
     * @brief Create and attach workspace dock widgets.
     * @param mainWindow Target main window
     * @param sidebarTabs Controls sidebar widget
     * @param summaryPane Telemetry summary pane
     * @param validationPane Validation pane
     */
    static void buildDocks(MainWindow* mainWindow, QTabWidget* sidebarTabs,
                          QWidget* summaryPane, QWidget* validationPane);
};

}  // namespace bltzr_qt::mainWindow::shell

#endif  // BLTZAR_QT_MAINWINDOW_SHELL_DOCKBUILDER_HPP
