/*
 * @file modules/qt/include/ui/mainWindow/fileActions/fileDialogs.hpp
 * @brief File dialog and save/load handling.
 */

#ifndef BLTZAR_QT_MAINWINDOW_FILEACTIONS_FILEDIAOGS_HPP
#define BLTZAR_QT_MAINWINDOW_FILEACTIONS_FILEDIAOGS_HPP

class MainWindow;

namespace bltzr_qt::mainWindow::fileActions {

/**
 * @brief Handle file dialogs for export, checkpoint, input, and preset loading.
 */
class FileDialogs final {
public:
    /**
     * @brief Handle export snapshot request.
     * @param mainWindow Target main window
     */
    static void handleExportRequest(MainWindow* mainWindow);

    /**
     * @brief Handle save checkpoint request.
     * @param mainWindow Target main window
     */
    static void handleSaveCheckpoint(MainWindow* mainWindow);

    /**
     * @brief Handle load checkpoint request.
     * @param mainWindow Target main window
     */
    static void handleLoadCheckpoint(MainWindow* mainWindow);

    /**
     * @brief Handle load input file request.
     * @param mainWindow Target main window
     */
    static void handleLoadInput(MainWindow* mainWindow);

    /**
     * @brief Handle load preset request.
     * @param mainWindow Target main window
     */
    static void handleLoadPreset(MainWindow* mainWindow);
};

}  // namespace bltzr_qt::mainWindow::fileActions

#endif  // BLTZAR_QT_MAINWINDOW_FILEACTIONS_FILEDIAOGS_HPP
