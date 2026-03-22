#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_QTTHEME_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_QTTHEME_HPP_

/*
 * Module: ui
 * Responsibility: Centralize the Qt workspace theme palette and stylesheet tokens.
 */

#include <QPalette>
#include <QString>

#include <string>

namespace grav_qt {

enum class QtThemeMode {
    Light,
    Dark
};

class QtTheme final {
    public:
        static QtThemeMode resolve(const std::string &themeName);
        static std::string toConfigValue(QtThemeMode mode);
        static QPalette buildPalette(QtThemeMode mode);
        static QString buildMainWindowStyleSheet(QtThemeMode mode);
};

} // namespace grav_qt

#endif // GRAVITY_MODULES_QT_INCLUDE_UI_QTTHEME_HPP_
