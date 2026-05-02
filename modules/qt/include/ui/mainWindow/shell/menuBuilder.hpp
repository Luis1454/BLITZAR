/*
 * @file modules/qt/include/ui/mainWindow/shell/menuBuilder.hpp
 * @brief Menu bar and menu construction utilities.
 */

#ifndef BLTZAR_QT_MAINWINDOW_SHELL_MENUBUILDER_HPP
#define BLTZAR_QT_MAINWINDOW_SHELL_MENUBUILDER_HPP

class MainWindow;

namespace bltzr_qt::mainWindow::shell {

/**
 * @brief Build and configure menu bar and menus.
 */
class MenuBuilder final {
public:
    /**
     * @brief Create menu bar with all application menus.
     * @param mainWindow Target main window
     */
    static void buildMenus(MainWindow* mainWindow);
};

}  // namespace bltzr_qt::mainWindow::shell

#endif  // BLTZAR_QT_MAINWINDOW_SHELL_MENUBUILDER_HPP
