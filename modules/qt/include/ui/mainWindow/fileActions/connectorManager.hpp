/*
 * @file modules/qt/include/ui/mainWindow/fileActions/connectorManager.hpp
 * @brief Runtime connector configuration management.
 */

#ifndef BLTZAR_QT_MAINWINDOW_FILEACTIONS_CONNECTORMANAGER_HPP
#define BLTZAR_QT_MAINWINDOW_FILEACTIONS_CONNECTORMANAGER_HPP

class MainWindow;

namespace bltzr_qt::mainWindow::fileActions {

/**
 * @brief Manage connector configuration and runtime updates.
 */
class ConnectorManager final {
public:
    /**
     * @brief Read UI connector fields and apply to runtime.
     * @param mainWindow Target main window with connector controls
     */
    static void configureConnector(MainWindow* mainWindow);

    /**
     * @brief Reset energy graph state on connection change.
     * @param mainWindow Target main window
     */
    static void resetEnergyGraphOnConnectorChange(MainWindow* mainWindow);
};

}  // namespace bltzr_qt::mainWindow::fileActions

#endif  // BLTZAR_QT_MAINWINDOW_FILEACTIONS_CONNECTORMANAGER_HPP
