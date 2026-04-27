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
enum class QtThemeMode { Light, Dark };
/// Description: Defines the QtTheme data or behavior contract.
class QtTheme final {
public:
    /// Description: Executes the resolve operation.
    static QtThemeMode resolve(const std::string& themeName);
    /// Description: Executes the toConfigValue operation.
    static std::string toConfigValue(QtThemeMode mode);
    /// Description: Executes the buildPalette operation.
    static QPalette buildPalette(QtThemeMode mode);
    /// Description: Executes the buildMainWindowStyleSheet operation.
    static QString buildMainWindowStyleSheet(QtThemeMode mode);
};
} // namespace grav_qt
#endif // GRAVITY_MODULES_QT_INCLUDE_UI_QTTHEME_HPP_
