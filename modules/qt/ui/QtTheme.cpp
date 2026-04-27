// File: modules/qt/ui/QtTheme.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "ui/QtTheme.hpp"
#include <QColor>

namespace grav_qt {
/// Description: Defines the QtThemeLocal data or behavior contract.
class QtThemeLocal final {
public:
    static QString lightStyleSheet()
    {
        return "QMainWindow { background: #edf2f7; }"
               "QMenuBar { background: #eef3f8; color: #102a43; border-bottom: 1px solid #cbd5e0; }"
               "QMenuBar::item { padding: 6px 10px; }"
               "QMenuBar::item:selected { background: #d7e3f1; color: #102a43; border-radius: 4px; "
               "}"
               "QMenu { background: #ffffff; color: #102a43; border: 1px solid #cbd5e0; }"
               "QMenu::item:selected { background: #d7e3f1; color: #102a43; }"
               "QDockWidget { color: #102a43; }"
               "QDockWidget::title { background: #dce6f2; color: #102a43; padding: 7px 10px; "
               "font-weight: 700; }"
               "QGroupBox { border: 1px solid #d3ddea; border-radius: 10px; margin-top: 10px; "
               "padding-top: 10px; background: #f8fbff; }"
               "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: "
               "#243b53; font-weight: 700; }"
               "QPushButton { background: #0f4c81; color: #f8fbff; border: 1px solid #0b3a60; "
               "border-radius: 8px; padding: 7px 10px; min-height: 18px; font-weight: 700; }"
               "QPushButton:hover { background: #1363a3; }"
               "QPushButton:pressed { background: #0b3a60; }"
               "QPushButton:checked { background: #114b5f; border-color: #0d3947; }"
               "QPushButton:disabled { background: #e7edf5; color: #7b8794; border: 1px solid "
               "#c7d2df; }"
               "QLineEdit, QComboBox, QDoubleSpinBox, QSpinBox { background: #ffffff; color: "
               "#102a43; border: 1px solid #b9c7d8; border-radius: 7px; padding: 4px 6px; }"
               "QLineEdit:disabled, QComboBox:disabled, QDoubleSpinBox:disabled, QSpinBox:disabled "
               "{ background: #f1f5f9; color: #7b8794; border-color: #d9e2ec; }"
               "QComboBox QAbstractItemView { color: #102a43; background: #ffffff; "
               "selection-background-color: #d7e3f1; }"
               "QLabel { color: #243b53; }"
               "QLabel#validationLabel { color: #8b1e1e; }"
               "QCheckBox { color: #243b53; spacing: 6px; }"
               "QCheckBox::indicator { width: 16px; height: 16px; border-radius: 4px; border: 1px "
               "solid #97a6ba; background: #ffffff; }"
               "QCheckBox::indicator:checked { background: #0f4c81; border-color: #0b3a60; image: "
               "none; }"
               "QCheckBox::indicator:unchecked:hover, QCheckBox::indicator:checked:hover { "
               "border-color: #1363a3; }"
               "QTabWidget::pane { border: 0; }"
               "QTabWidget#workspaceSidebarTabs QTabBar::tab { background: #d7e1ed; color: "
               "#486581; border-radius: 8px; padding: 10px 10px; margin: 2px; font-weight: 600; "
               "min-width: 58px; min-height: 58px; }"
               "QTabWidget#workspaceSidebarTabs QTabBar::tab:selected { background: #ffffff; "
               "color: #102a43; font-weight: 700; }"
               "QStatusBar { background: #f8fafc; border-top: 1px solid #d9e2ec; color: #486581; }"
               "QWidget#telemetrySummaryPane QFrame#runtimeCard { border: 1px solid #d7dee6; "
               "border-radius: 8px; background: #f7f9fb; }"
               "QWidget#telemetrySummaryPane QLabel#runtimeCardTitle { color: #486581; font-size: "
               "11px; font-weight: 700; text-transform: uppercase; }"
               "QWidget#telemetrySummaryPane QLabel#runtimeSummaryValue { color: #102a43; }";
    }

    static QString darkStyleSheet()
    {
        return "QMainWindow { background: #101826; }"
               "QMenuBar { background: #162133; color: #e5edf5; border-bottom: 1px solid #243447; }"
               "QMenuBar::item { padding: 6px 10px; }"
               "QMenuBar::item:selected { background: #243447; color: #f8fbff; border-radius: 4px; "
               "}"
               "QMenu { background: #182436; color: #e5edf5; border: 1px solid #2a3b52; }"
               "QMenu::item:selected { background: #2b4363; color: #f8fbff; }"
               "QDockWidget { color: #d9e2ec; }"
               "QDockWidget::title { background: #1a273a; color: #f0f4f8; padding: 7px 10px; "
               "font-weight: 700; }"
               "QGroupBox { border: 1px solid #2a3b52; border-radius: 10px; margin-top: 10px; "
               "padding-top: 10px; background: #162133; }"
               "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: "
               "#cdd8e5; font-weight: 700; }"
               "QPushButton { background: #2b6cb0; color: #f8fbff; border: 1px solid #215285; "
               "border-radius: 8px; padding: 7px 10px; min-height: 18px; font-weight: 700; }"
               "QPushButton:hover { background: #3680d1; }"
               "QPushButton:pressed { background: #215285; }"
               "QPushButton:checked { background: #1f5e75; border-color: #194959; }"
               "QPushButton:disabled { background: #223043; color: #7b8794; border: 1px solid "
               "#304156; }"
               "QLineEdit, QComboBox, QDoubleSpinBox, QSpinBox { background: #0f1724; color: "
               "#e5edf5; border: 1px solid #39506d; border-radius: 7px; padding: 4px 6px; }"
               "QLineEdit:disabled, QComboBox:disabled, QDoubleSpinBox:disabled, QSpinBox:disabled "
               "{ background: #172131; color: #7b8794; border-color: #243447; }"
               "QComboBox QAbstractItemView { color: #e5edf5; background: #182436; "
               "selection-background-color: #2b4363; }"
               "QLabel { color: #d9e2ec; }"
               "QLabel#validationLabel { color: #ff8c8c; }"
               "QCheckBox { color: #d9e2ec; spacing: 6px; }"
               "QCheckBox::indicator { width: 16px; height: 16px; border-radius: 4px; border: 1px "
               "solid #5a708a; background: #101826; }"
               "QCheckBox::indicator:checked { background: #2b6cb0; border-color: #215285; image: "
               "none; }"
               "QCheckBox::indicator:unchecked:hover, QCheckBox::indicator:checked:hover { "
               "border-color: #5da0ff; }"
               "QTabWidget::pane { border: 0; }"
               "QTabWidget#workspaceSidebarTabs QTabBar::tab { background: #223247; color: "
               "#9fb3c8; border-radius: 8px; padding: 10px 10px; margin: 2px; font-weight: 600; "
               "min-width: 58px; min-height: 58px; }"
               "QTabWidget#workspaceSidebarTabs QTabBar::tab:selected { background: #101826; "
               "color: #f0f4f8; font-weight: 700; }"
               "QStatusBar { background: #162133; border-top: 1px solid #243447; color: #9fb3c8; }"
               "QWidget#telemetrySummaryPane QFrame#runtimeCard { border: 1px solid #2a3b52; "
               "border-radius: 8px; background: #162133; }"
               "QWidget#telemetrySummaryPane QLabel#runtimeCardTitle { color: #8ea2b8; font-size: "
               "11px; font-weight: 700; text-transform: uppercase; }"
               "QWidget#telemetrySummaryPane QLabel#runtimeSummaryValue { color: #f0f4f8; }";
    }
};

/// Description: Executes the resolve operation.
QtThemeMode QtTheme::resolve(const std::string& themeName)
{
    return themeName == "dark" ? QtThemeMode::Dark : QtThemeMode::Light;
}

/// Description: Executes the toConfigValue operation.
std::string QtTheme::toConfigValue(QtThemeMode mode)
{
    return mode == QtThemeMode::Dark ? "dark" : "light";
}

/// Description: Executes the buildPalette operation.
QPalette QtTheme::buildPalette(QtThemeMode mode)
{
    QPalette palette;
    if (mode == QtThemeMode::Dark) {
        palette.setColor(QPalette::Window, QColor(16, 24, 38));
        palette.setColor(QPalette::WindowText, QColor(229, 237, 245));
        palette.setColor(QPalette::Base, QColor(15, 23, 36));
        palette.setColor(QPalette::AlternateBase, QColor(24, 36, 54));
        palette.setColor(QPalette::Text, QColor(229, 237, 245));
        palette.setColor(QPalette::Button, QColor(22, 33, 51));
        palette.setColor(QPalette::ButtonText, QColor(240, 244, 248));
        palette.setColor(QPalette::Mid, QColor(90, 112, 138));
        palette.setColor(QPalette::Highlight, QColor(43, 108, 176));
        palette.setColor(QPalette::HighlightedText, QColor(248, 251, 255));
        palette.setColor(QPalette::ToolTipBase, QColor(24, 36, 54));
        palette.setColor(QPalette::ToolTipText, QColor(240, 244, 248));
        palette.setColor(QPalette::PlaceholderText, QColor(139, 156, 173));
        return palette;
    }
    palette.setColor(QPalette::Window, QColor(237, 242, 247));
    palette.setColor(QPalette::WindowText, QColor(16, 42, 67));
    palette.setColor(QPalette::Base, QColor(255, 255, 255));
    palette.setColor(QPalette::AlternateBase, QColor(247, 249, 251));
    palette.setColor(QPalette::Text, QColor(16, 42, 67));
    palette.setColor(QPalette::Button, QColor(220, 230, 242));
    palette.setColor(QPalette::ButtonText, QColor(16, 42, 67));
    palette.setColor(QPalette::Mid, QColor(155, 160, 170));
    palette.setColor(QPalette::Highlight, QColor(15, 76, 129));
    palette.setColor(QPalette::HighlightedText, QColor(248, 251, 255));
    palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    palette.setColor(QPalette::ToolTipText, QColor(16, 42, 67));
    palette.setColor(QPalette::PlaceholderText, QColor(123, 135, 148));
    return palette;
}

QString QtTheme::buildMainWindowStyleSheet(QtThemeMode mode)
{
    return mode == QtThemeMode::Dark ? QtThemeLocal::darkStyleSheet()
                                     : QtThemeLocal::lightStyleSheet();
}
} // namespace grav_qt
