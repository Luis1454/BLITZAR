/*
 * @file modules/qt/include/ui/WorkspaceTheme.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_QTTHEME_HPP_
#define BLITZAR_MODULES_QT_INCLUDE_UI_QTTHEME_HPP_
/*
 * Module: ui
 * Responsibility: Centralize the Qt workspace theme palette tokens.
 */
#include <QPalette>
#include <string>

namespace bltzr_qt {
enum class QtThemeMode {
    Light,
    Dark
};

class WorkspaceTheme final {
public:
    static QtThemeMode resolve(const std::string& themeName);
    static std::string toConfigValue(QtThemeMode mode);
    static QPalette buildPalette(QtThemeMode mode);
};
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_INCLUDE_UI_QTTHEME_HPP_
