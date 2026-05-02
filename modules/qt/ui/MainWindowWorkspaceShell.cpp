/*
 * @file modules/qt/ui/MainWindowWorkspaceShell.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Thin facade delegating to shell subfolder modules.
 */

#include "ui/MainWindow.hpp"

namespace bltzr_qt {

void MainWindow::buildWorkspaceDocks(QTabWidget* sidebarTabs, QWidget* summaryPane,
                                     QWidget* validationPane)
{
    // TODO: Integrate dockBuilder module for full implementation
}

void MainWindow::buildMenus()
{
    // TODO: Integrate menuBuilder and themeMenuHandler modules for full implementation
}

}  // namespace bltzr_qt
