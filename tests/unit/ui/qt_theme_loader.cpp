/*
 * @file tests/unit/ui/qt_theme_loader.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "ui/ThemeLoader.hpp"

#include <QFile>
#include <QIODevice>
#include <QPalette>
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <optional>
#include <string>

namespace bltzr_test_qt_ui {

static std::filesystem::path makeThemeTempFile(const std::string& content)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / ("BLITZAR_theme_" + std::to_string(stamp));
    std::filesystem::create_directories(root);
    const std::filesystem::path filePath = root / "theme.json";
    QFile file(QString::fromUtf8(filePath.u8string().c_str()));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }
    const qint64 bytesWritten = file.write(content.c_str(), static_cast<qint64>(content.size()));
    file.close();
    if (bytesWritten != static_cast<qint64>(content.size())) {
        return {};
    }
    return filePath;
}

TEST(QtUiLogicTest, TST_UNT_UI_008_ThemeLoaderLoadsValidCustomTheme)
{
    const std::filesystem::path filePath = makeThemeTempFile(R"({
        "schemaVersion": 1,
        "name": "custom-dark",
        "base": "dark",
        "palette": {
            "Window": "#101820",
            "WindowText": "#f0f4f8",
            "Highlight": "#ff7a18"
        },
        "stylesheet": "QMainWindow { border: 0; }"
    })");

    ASSERT_FALSE(filePath.empty());
    const std::optional<bltzr_qt::ThemeSpec> theme = bltzr_qt::ThemeLoader::loadFromFile(filePath);
    ASSERT_TRUE(theme.has_value());
    EXPECT_EQ(theme->name().toStdString(), "custom-dark");
    EXPECT_EQ(theme->base(), bltzr_qt::ThemeBase::Dark);
    EXPECT_EQ(theme->palette().color(QPalette::Window).name().toStdString(), "#101820");
    EXPECT_EQ(theme->palette().color(QPalette::WindowText).name().toStdString(), "#f0f4f8");
    EXPECT_EQ(theme->palette().color(QPalette::Highlight).name().toStdString(), "#ff7a18");
    EXPECT_EQ(theme->stylesheet().toStdString(), "QMainWindow { border: 0; }");

    std::error_code ec;
    std::filesystem::remove_all(filePath.parent_path(), ec);
}

TEST(QtUiLogicTest, TST_UNT_UI_009_ThemeLoaderRejectsCorruptedThemeFiles)
{
    const std::filesystem::path invalidJson = makeThemeTempFile("{ not valid json }");
    const std::filesystem::path wrongSchema = makeThemeTempFile(R"({
        "schemaVersion": 2,
        "name": "broken",
        "base": "dark",
        "palette": {}
    })");
    const std::filesystem::path badRole = makeThemeTempFile(R"({
        "schemaVersion": 1,
        "name": "broken",
        "base": "dark",
        "palette": {
            "UnknownRole": "#ffffff"
        }
    })");
    const std::filesystem::path badColor = makeThemeTempFile(R"({
        "schemaVersion": 1,
        "name": "broken",
        "base": "light",
        "palette": {
            "Window": "not-a-color"
        }
    })");

    EXPECT_FALSE(bltzr_qt::ThemeLoader::loadFromFile(invalidJson).has_value());
    EXPECT_FALSE(bltzr_qt::ThemeLoader::loadFromFile(wrongSchema).has_value());
    EXPECT_FALSE(bltzr_qt::ThemeLoader::loadFromFile(badRole).has_value());
    EXPECT_FALSE(bltzr_qt::ThemeLoader::loadFromFile(badColor).has_value());

    std::error_code ec;
    std::filesystem::remove_all(invalidJson.parent_path(), ec);
    std::filesystem::remove_all(wrongSchema.parent_path(), ec);
    std::filesystem::remove_all(badRole.parent_path(), ec);
    std::filesystem::remove_all(badColor.parent_path(), ec);
}

TEST(QtUiLogicTest, TST_UNT_UI_010_ThemeLoaderUsesExplicitDefaultThemeFiles)
{
    const std::optional<bltzr_qt::ThemeSpec> lightTheme =
        bltzr_qt::ThemeLoader::loadDefaultTheme(bltzr_qt::ThemeBase::Light);
    ASSERT_TRUE(lightTheme.has_value());
    EXPECT_EQ(lightTheme->name().toStdString(), "default-light");
    EXPECT_EQ(lightTheme->base(), bltzr_qt::ThemeBase::Light);

    const std::optional<bltzr_qt::ThemeSpec> darkTheme =
        bltzr_qt::ThemeLoader::loadDefaultTheme(bltzr_qt::ThemeBase::Dark);
    ASSERT_TRUE(darkTheme.has_value());
    EXPECT_EQ(darkTheme->name().toStdString(), "default-dark");
    EXPECT_EQ(darkTheme->base(), bltzr_qt::ThemeBase::Dark);
}

} // namespace bltzr_test_qt_ui