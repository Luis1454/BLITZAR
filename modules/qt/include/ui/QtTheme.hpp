// File: modules/qt/include/ui/QtTheme.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

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
/// Description: Enumerates the supported QtThemeMode values.
enum class QtThemeMode {
    Light,
    Dark
};

/// Description: Defines the QtTheme data or behavior contract.
class QtTheme final {
public:
    /// Description: Describes the resolve operation contract.
    static QtThemeMode resolve(const std::string& themeName);
    /// Description: Describes the to config value operation contract.
    static std::string toConfigValue(QtThemeMode mode);
    /// Description: Describes the build palette operation contract.
    static QPalette buildPalette(QtThemeMode mode);
    /// Description: Describes the build main window style sheet operation contract.
    static QString buildMainWindowStyleSheet(QtThemeMode mode);
};
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_QTTHEME_HPP_
