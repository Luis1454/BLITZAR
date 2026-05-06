/*
 * @file modules/qt/ui/WorkspaceTheme.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/QtTheme.hpp"
#include "ui/ThemeLoader.hpp"

#include <QColor>

static void applySharedPalette(QPalette& palette)
{
    palette.setColor(QPalette::HighlightedText, QColor(248, 251, 255));
    palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    palette.setColor(QPalette::ToolTipText, QColor(16, 42, 67));
}

static void applyLightPalette(QPalette& palette)
{
    palette.setColor(QPalette::Window, QColor(237, 242, 247));
    palette.setColor(QPalette::WindowText, QColor(16, 42, 67));
    palette.setColor(QPalette::Base, QColor(255, 255, 255));
    palette.setColor(QPalette::AlternateBase, QColor(247, 249, 251));
    palette.setColor(QPalette::Text, QColor(16, 42, 67));
    palette.setColor(QPalette::Button, QColor(220, 230, 242));
    palette.setColor(QPalette::ButtonText, QColor(16, 42, 67));
    palette.setColor(QPalette::Mid, QColor(155, 160, 170));
    palette.setColor(QPalette::Highlight, QColor(15, 76, 129));
    palette.setColor(QPalette::PlaceholderText, QColor(123, 135, 148));
    applySharedPalette(palette);
}

static void applyDarkPalette(QPalette& palette)
{
    palette.setColor(QPalette::Window, QColor(16, 24, 38));
    palette.setColor(QPalette::WindowText, QColor(229, 237, 245));
    palette.setColor(QPalette::Base, QColor(15, 23, 36));
    palette.setColor(QPalette::AlternateBase, QColor(24, 36, 54));
    palette.setColor(QPalette::Text, QColor(229, 237, 245));
    palette.setColor(QPalette::Button, QColor(22, 33, 51));
    palette.setColor(QPalette::ButtonText, QColor(240, 244, 248));
    palette.setColor(QPalette::Mid, QColor(90, 112, 138));
    palette.setColor(QPalette::Highlight, QColor(43, 108, 176));
    palette.setColor(QPalette::PlaceholderText, QColor(139, 156, 173));
    applySharedPalette(palette);
}

namespace bltzr_qt {
QtThemeMode WorkspaceTheme::resolve(const std::string& themeName)
{
    return themeName == "dark" ? QtThemeMode::Dark : QtThemeMode::Light;
}

std::string WorkspaceTheme::toConfigValue(QtThemeMode mode)
{
    return mode == QtThemeMode::Dark ? "dark" : "light";
}

QPalette WorkspaceTheme::buildPalette(QtThemeMode mode)
{
    const bltzr_qt::ThemeBase themeBase = (mode == QtThemeMode::Dark) ? ThemeBase::Dark : ThemeBase::Light;
    const std::optional<ThemeSpec> loadedTheme = ThemeLoader::loadDefaultTheme(themeBase);
    if (loadedTheme.has_value()) {
        return loadedTheme->palette();
    }

    QPalette palette;
    if (mode == QtThemeMode::Dark) {
        applyDarkPalette(palette);
    }
    else {
        applyLightPalette(palette);
    }
    return palette;
}

} // namespace bltzr_qt
