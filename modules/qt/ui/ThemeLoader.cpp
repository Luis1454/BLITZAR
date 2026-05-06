/*
 * @file modules/qt/ui/ThemeLoader.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Strict JSON theme loader implementations.
 */

#include "ui/ThemeLoader.hpp"

#include <QColor>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include <cstdlib>
#include <initializer_list>

#ifndef BLITZAR_QT_THEME_DIR
#define BLITZAR_QT_THEME_DIR ""
#endif

namespace bltzr_qt {

const QString& ThemeSpec::name() const
{
    return _name;
}

ThemeBase ThemeSpec::base() const
{
    return _base;
}

const QPalette& ThemeSpec::palette() const
{
    return _palette;
}

const QString& ThemeSpec::stylesheet() const
{
    return _stylesheet;
}

static void applyThemeCommon(QPalette& palette)
{
    palette.setColor(QPalette::HighlightedText, QColor(248, 251, 255));
    palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    palette.setColor(QPalette::ToolTipText, QColor(16, 42, 67));
}

static void applyLightTheme(QPalette& palette)
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
    applyThemeCommon(palette);
}

static void applyDarkTheme(QPalette& palette)
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
    applyThemeCommon(palette);
}

static std::optional<QPalette::ColorRole> colorRoleFromName(const QString& roleName)
{
    if (roleName == "Window") {
        return QPalette::Window;
    }
    if (roleName == "WindowText") {
        return QPalette::WindowText;
    }
    if (roleName == "Base") {
        return QPalette::Base;
    }
    if (roleName == "AlternateBase") {
        return QPalette::AlternateBase;
    }
    if (roleName == "Text") {
        return QPalette::Text;
    }
    if (roleName == "Button") {
        return QPalette::Button;
    }
    if (roleName == "ButtonText") {
        return QPalette::ButtonText;
    }
    if (roleName == "Mid") {
        return QPalette::Mid;
    }
    if (roleName == "Highlight") {
        return QPalette::Highlight;
    }
    if (roleName == "HighlightedText") {
        return QPalette::HighlightedText;
    }
    if (roleName == "ToolTipBase") {
        return QPalette::ToolTipBase;
    }
    if (roleName == "ToolTipText") {
        return QPalette::ToolTipText;
    }
    if (roleName == "PlaceholderText") {
        return QPalette::PlaceholderText;
    }
    return std::nullopt;
}

static bool containsOnlyAllowedKeys(const QJsonObject& object,
                                    std::initializer_list<QString> allowedKeys)
{
    for (const QString& key : object.keys()) {
        bool allowed = false;
        for (const QString& expected : allowedKeys) {
            if (key == expected) {
                allowed = true;
                break;
            }
        }
        if (!allowed) {
            return false;
        }
    }
    return true;
}

static QPalette applyPaletteOverrides(const QPalette& basePalette, const QJsonObject& paletteObject,
                                      bool* ok)
{
    QPalette palette = basePalette;
    *ok = true;
    for (const QString& roleName : paletteObject.keys()) {
        const std::optional<QPalette::ColorRole> role = colorRoleFromName(roleName);
        if (!role.has_value()) {
            *ok = false;
            return palette;
        }
        const QJsonValue value = paletteObject.value(roleName);
        if (!value.isString()) {
            *ok = false;
            return palette;
        }
        const QColor color(value.toString());
        if (!color.isValid()) {
            *ok = false;
            return palette;
        }
        palette.setColor(*role, color);
    }
    return palette;
}

ThemeSpec ThemeLoader::builtinTheme(ThemeBase base)
{
    ThemeSpec theme;
    theme._base = base;
    theme._name = base == ThemeBase::Dark ? QStringLiteral("dark") : QStringLiteral("light");
    if (base == ThemeBase::Dark) {
        applyDarkTheme(theme._palette);
    }
    else {
        applyLightTheme(theme._palette);
    }
    return theme;
}

std::filesystem::path ThemeLoader::defaultThemePath(ThemeBase base)
{
    const std::filesystem::path themeDirectory = std::filesystem::path(BLITZAR_QT_THEME_DIR);
    if (base == ThemeBase::Dark) {
        return themeDirectory / "default-dark.json";
    }
    return themeDirectory / "default-light.json";
}

std::optional<ThemeSpec> ThemeLoader::loadDefaultTheme(ThemeBase base)
{
    return loadFromFile(defaultThemePath(base));
}

std::optional<ThemeSpec> ThemeLoader::loadFromFile(const std::filesystem::path& path)
{
    QFile file(QString::fromUtf8(path.u8string().c_str()));
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    return loadFromJson(file.readAll());
}

std::optional<ThemeSpec> ThemeLoader::loadFromJson(const QByteArray& jsonBytes)
{
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(jsonBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return std::nullopt;
    }

    const QJsonObject root = document.object();
    if (!containsOnlyAllowedKeys(root, {QStringLiteral("schemaVersion"), QStringLiteral("name"),
                                        QStringLiteral("base"), QStringLiteral("palette"),
                                        QStringLiteral("stylesheet")})) {
        return std::nullopt;
    }

    if (!root.value("schemaVersion").isDouble() || root.value("schemaVersion").toInt() != 1) {
        return std::nullopt;
    }
    if (!root.value("name").isString() || root.value("name").toString().trimmed().isEmpty()) {
        return std::nullopt;
    }
    if (!root.value("base").isString() || !root.value("palette").isObject()) {
        return std::nullopt;
    }

    const QString baseName = root.value("base").toString();
    if (baseName != QStringLiteral("light") && baseName != QStringLiteral("dark")) {
        return std::nullopt;
    }

    const QJsonObject paletteObject = root.value("palette").toObject();
    if (!containsOnlyAllowedKeys(
            paletteObject,
            {QStringLiteral("Window"), QStringLiteral("WindowText"), QStringLiteral("Base"),
             QStringLiteral("AlternateBase"), QStringLiteral("Text"), QStringLiteral("Button"),
             QStringLiteral("ButtonText"), QStringLiteral("Mid"), QStringLiteral("Highlight"),
             QStringLiteral("HighlightedText"), QStringLiteral("ToolTipBase"),
             QStringLiteral("ToolTipText"), QStringLiteral("PlaceholderText")})) {
        return std::nullopt;
    }

    ThemeSpec theme =
        builtinTheme(baseName == QStringLiteral("dark") ? ThemeBase::Dark : ThemeBase::Light);
    theme._name = root.value("name").toString();

    bool overridesValid = false;
    theme._palette = applyPaletteOverrides(theme._palette, paletteObject, &overridesValid);
    if (!overridesValid) {
        return std::nullopt;
    }

    const QJsonValue stylesheetValue = root.value("stylesheet");
    if (!stylesheetValue.isUndefined() && !stylesheetValue.isString()) {
        return std::nullopt;
    }
    if (stylesheetValue.isString()) {
        theme._stylesheet = stylesheetValue.toString();
    }

    return theme;
}

} // namespace bltzr_qt