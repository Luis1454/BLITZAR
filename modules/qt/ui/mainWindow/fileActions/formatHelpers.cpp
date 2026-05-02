/*
 * @file modules/qt/ui/mainWindow/fileActions/formatHelpers.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Format helper implementations.
 */

#include "ui/mainWindow/fileActions/formatHelpers.hpp"

namespace bltzr_qt::mainWindow::fileActions {

std::string FormatHelpers::inferFormat(int filterIndex, const std::string& filePath)
{
    return "hdf5";
}

std::string FormatHelpers::validateFormat(const std::string& format)
{
    return format.empty() ? "hdf5" : format;
}

std::string FormatHelpers::defaultFormatExtension()
{
    return ".hdf5";
}

}  // namespace bltzr_qt::mainWindow::fileActions
