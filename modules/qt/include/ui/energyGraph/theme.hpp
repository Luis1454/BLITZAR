/*
 * @file modules/qt/include/ui/energyGraph/theme.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Theme and color palette management for energy graph widget.
 */

#ifndef BLTZR_QT_ENERGY_GRAPH_THEME_HPP
#define BLTZR_QT_ENERGY_GRAPH_THEME_HPP

#include <QColor>
#include <QPalette>

namespace bltzr_qt::energyGraph {

/**
 * @brief Theme color palette for energy graph visualization.
 */
struct ColorPalette {
    QColor background;
    QColor border;
    QColor label;
    QColor grid;
    QColor kineticCurve;
    QColor potentialCurve;
    QColor thermalCurve;
    QColor radiatedCurve;
    QColor totalCurve;
    QColor driftCurve;
};

/**
 * @brief Detect if a palette uses dark theme.
 * @param palette The QPalette to analyze
 * @return true if dark theme, false if light theme
 */
bool isDarkThemeEnabled(const QPalette& palette);

/**
 * @brief Build complete color palette from widget palette.
 * @param widgetPalette The widget's palette
 * @return Fully populated ColorPalette
 */
ColorPalette buildColorPalette(const QPalette& widgetPalette);

} // namespace bltzr_qt::energyGraph

#endif // BLTZR_QT_ENERGY_GRAPH_THEME_HPP
