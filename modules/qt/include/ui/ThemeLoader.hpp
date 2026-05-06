/*
 * @file modules/qt/include/ui/ThemeLoader.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Strict loaders for declarative Qt workspace themes.
 */

#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_THEMELOADER_HPP_
#define BLITZAR_MODULES_QT_INCLUDE_UI_THEMELOADER_HPP_

#include <QByteArray>
#include <QPalette>
#include <QString>
#include <filesystem>
#include <optional>

namespace bltzr_qt {

enum class ThemeBase {
    Light,
    Dark
};

class ThemeSpec final {
public:
    const QString& name() const;
    ThemeBase base() const;
    const QPalette& palette() const;
    const QString& stylesheet() const;

private:
    friend class ThemeLoader;

    QString _name;
    ThemeBase _base = ThemeBase::Light;
    QPalette _palette;
    QString _stylesheet;
};

class ThemeLoader final {
public:
    static ThemeSpec builtinTheme(ThemeBase base);
    static std::filesystem::path defaultThemePath(ThemeBase base);
    static std::optional<ThemeSpec> loadDefaultTheme(ThemeBase base);
    static std::optional<ThemeSpec> loadFromFile(const std::filesystem::path& path);
    static std::optional<ThemeSpec> loadFromJson(const QByteArray& jsonBytes);
};

} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_INCLUDE_UI_THEMELOADER_HPP_