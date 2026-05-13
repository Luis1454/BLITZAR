/*
 * @file modules/qt/src/support/theme/Theme.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#ifndef BLITZAR_MODULES_QT_SRC_SUPPORT_THEME_THEME_HPP_
#define BLITZAR_MODULES_QT_SRC_SUPPORT_THEME_THEME_HPP_
/*
 * Module: qt
 * Responsibility: Centralize the Qt workspace theme palette tokens.
 */
#include <QPalette>
#include <string>

namespace bltzr_qt {
enum class ThemeMode {
    Light,
    Dark
};

class Theme final {
public:
    static ThemeMode resolve(const std::string& themeName);
    static std::string toConfigValue(ThemeMode mode);
    static QPalette buildPalette(ThemeMode mode);
};
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_SRC_SUPPORT_THEME_THEME_HPP_
