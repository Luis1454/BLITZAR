/*
 * @file modules/qt/include/ui/mainWindow/shell/themeMenuHandler.hpp
 * @brief Theme menu and action group management.
 */

#ifndef BLTZAR_QT_MAINWINDOW_SHELL_THEMEMENUHANDLER_HPP
#define BLTZAR_QT_MAINWINDOW_SHELL_THEMEMENUHANDLER_HPP

class QActionGroup;
class QMenu;
class MainWindow;

namespace bltzr_qt::mainWindow::shell {

/**
 * @brief Manage theme selection menu and exclusive action group.
 */
class ThemeMenuHandler final {
public:
    /**
     * @brief Create theme menu with exclusive action group.
     * @param mainWindow Target main window
     * @param parentMenu Parent menu to add theme submenu to
     * @return Action group for theme selection
     */
    static QActionGroup* buildThemeMenu(MainWindow* mainWindow, QMenu* parentMenu);
};

}  // namespace bltzr_qt::mainWindow::shell

#endif  // BLTZAR_QT_MAINWINDOW_SHELL_THEMEMENUHANDLER_HPP
