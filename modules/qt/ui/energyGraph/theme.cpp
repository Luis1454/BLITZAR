/*
 * @file modules/qt/ui/energyGraph/theme.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Theme and color palette management for energy graph widget.
 */

#include "ui/energyGraph/theme.hpp"
#include <QColor>
#include <QPalette>

namespace bltzr_qt::energyGraph {

bool isDarkThemeEnabled(const QPalette& palette)
{
    return palette.color(QPalette::Window).lightness() < 128;
}

static QColor selectCurveColor(const QColor& darkColor, const QColor& lightColor,
                               const bool darkTheme)
{
    return darkTheme ? darkColor : lightColor;
}

ColorPalette buildColorPalette(const QPalette& widgetPalette)
{
    const bool darkTheme = isDarkThemeEnabled(widgetPalette);

    QColor gridColor = widgetPalette.color(QPalette::Mid);
    gridColor.setAlpha(darkTheme ? 190 : 120);

    return ColorPalette{
        widgetPalette.color(QPalette::Window),
        widgetPalette.color(QPalette::Mid),
        widgetPalette.color(QPalette::WindowText),
        gridColor,
        selectCurveColor(QColor(92, 255, 140), QColor(0, 122, 52), darkTheme),
        selectCurveColor(QColor(255, 120, 108), QColor(180, 45, 35), darkTheme),
        selectCurveColor(QColor(255, 170, 90), QColor(176, 99, 10), darkTheme),
        selectCurveColor(QColor(180, 120, 255), QColor(108, 52, 188), darkTheme),
        selectCurveColor(QColor(120, 200, 255), QColor(0, 102, 170), darkTheme),
        selectCurveColor(QColor(255, 230, 120), QColor(168, 132, 0), darkTheme),
    };
}

} // namespace bltzr_qt::energyGraph
