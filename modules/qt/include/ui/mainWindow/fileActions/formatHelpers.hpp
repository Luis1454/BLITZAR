/*
 * @file modules/qt/include/ui/mainWindow/fileActions/formatHelpers.hpp
 * @brief File format inference and handling utilities.
 */

#ifndef BLTZAR_QT_MAINWINDOW_FILEACTIONS_FORMATHELPERS_HPP
#define BLTZAR_QT_MAINWINDOW_FILEACTIONS_FORMATHELPERS_HPP

#include <string>

namespace bltzr_qt::mainWindow::fileActions {

/**
 * @brief Infer and validate file formats from dialogs and paths.
 */
class FormatHelpers final {
public:
    /**
     * @brief Infer format from dialog filter index or file extension.
     * @param filterIndex Qt file dialog filter index
     * @param filePath File path to check extension
     * @return Format string (e.g., "hdf5")
     */
    static std::string inferFormat(int filterIndex, const std::string& filePath);

    /**
     * @brief Validate format and apply default if invalid.
     * @param format Format string to validate
     * @return Valid format string
     */
    static std::string validateFormat(const std::string& format);

    /**
     * @brief Get default format extension.
     * @return Default extension (e.g., ".hdf5")
     */
    static std::string defaultFormatExtension();
};

}  // namespace bltzr_qt::mainWindow::fileActions

#endif  // BLTZAR_QT_MAINWINDOW_FILEACTIONS_FORMATHELPERS_HPP
