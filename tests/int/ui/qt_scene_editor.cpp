/*
 * @file tests/int/ui/qt_scene_editor.cpp
 * @brief In-process, non-invasive GUI actions for scene object properties.
 */

#include "window/scene/SceneEditor.hpp"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QSysInfo>
#include <QTabWidget>
#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>

namespace bltzr_test_qt_scene_editor {
namespace blitzar_test_qt_scene_editor {
SimulationConfig makeSceneEditorConfig()
{
    SimulationConfig config{};
    SceneObjectConfig object;
    object.id = "galaxy";
    object.name = "Galaxy";
    object.type = "galaxy";
    object.particleCount = 16u;
    config.scene.objects.push_back(object);
    return config;
}

} // namespace blitzar_test_qt_scene_editor

TEST(QtSceneEditorTest, TST_UIX_UI_027_AddsGranularObjectPropertiesIndependently)
{
    int argc = 1;
    char applicationName[] = "blitzarQtSceneEditorGTests";
    char* argv[] = {applicationName, nullptr};
    const std::filesystem::path workingDirectory = std::filesystem::current_path();
    const std::filesystem::path platformCandidates[] = {
        workingDirectory / "platforms", workingDirectory / ".." / "platforms",
        workingDirectory / ".." / ".." / "platforms"};
    for (const std::filesystem::path& candidate : platformCandidates) {
        const QString deployedPlatforms =
            QString::fromStdString(candidate.lexically_normal().string());
        if (QDir(deployedPlatforms).exists()) {
            qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", deployedPlatforms.toUtf8());
            break;
        }
    }
    if (QSysInfo::productType() == "windows" && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "windows");
    QApplication application(argc, argv);
    bltzr_qt::SceneEditor editor(blitzar_test_qt_scene_editor::makeSceneEditorConfig());
    editor.setAttribute(Qt::WA_DontShowOnScreen);
    editor.resize(360, 800);
    editor.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);

    ASSERT_NE(editor.findChild<QPushButton*>("sceneObjectPropertyButton"), nullptr);
    auto* splitter = editor.findChild<QSplitter*>("sceneObjectEditorSplitter");
    ASSERT_NE(splitter, nullptr);
    ASSERT_EQ(splitter->orientation(), Qt::Vertical);
    ASSERT_EQ(splitter->count(), 2);
    auto* objectList = editor.findChild<QListWidget*>("sceneObjectList");
    ASSERT_NE(objectList, nullptr);
    EXPECT_GE(objectList->minimumHeight(), 100);
    auto* objectCount = editor.findChild<QLabel*>("sceneObjectCountLabel");
    ASSERT_NE(objectCount, nullptr);
    EXPECT_EQ(objectCount->text().toStdString(), "1 object");
    auto* inspectorTabs = editor.findChild<QTabWidget*>("sceneObjectInspector");
    ASSERT_NE(inspectorTabs, nullptr);
    ASSERT_EQ(inspectorTabs->count(), 3);
    EXPECT_EQ(inspectorTabs->tabText(0).toStdString(), "Generator");
    EXPECT_EQ(inspectorTabs->tabText(1).toStdString(), "Transform");
    EXPECT_EQ(inspectorTabs->tabText(2).toStdString(), "Properties");
    auto* inspectorTitle = editor.findChild<QLabel*>("sceneObjectInspectorTitle");
    ASSERT_NE(inspectorTitle, nullptr);
    EXPECT_EQ(inspectorTitle->text().toStdString(), "Object inspector");
    auto* inspectorSelection = editor.findChild<QLabel*>("sceneObjectInspectorSelection");
    ASSERT_NE(inspectorSelection, nullptr);
    EXPECT_EQ(inspectorSelection->text().toStdString(), "Galaxy");
    auto* inspectorMeta = editor.findChild<QLabel*>("sceneObjectInspectorMeta");
    ASSERT_NE(inspectorMeta, nullptr);
    EXPECT_NE(inspectorMeta->text().toStdString().find("galaxy"), std::string::npos);
    const auto* inspector = inspectorTabs;
    EXPECT_LT(objectList->mapTo(&editor, QPoint(0, 0)).y(),
              inspector->mapTo(&editor, QPoint(0, 0)).y());
    EXPECT_FALSE(editor.findChild<QPushButton*>("applySceneObjectsButton")->isHidden());
    ASSERT_TRUE(editor.addObjectProperty(0, "mirror", true));
    ASSERT_FALSE(editor.findChild<QGroupBox*>("sceneObjectMirrorProperty")->isHidden());
    EXPECT_EQ(inspectorTabs->currentIndex(), 1);
    SceneConfig mirrorScene = editor.sceneConfiguration();
    ASSERT_EQ(mirrorScene.objects.size(), 1u);
    EXPECT_NE(std::find(mirrorScene.objects[0].properties.begin(),
                        mirrorScene.objects[0].properties.end(), "mirror"),
              mirrorScene.objects[0].properties.end());
    EXPECT_EQ(std::find(mirrorScene.objects[0].properties.begin(),
                        mirrorScene.objects[0].properties.end(), "rotation"),
              mirrorScene.objects[0].properties.end());

    ASSERT_TRUE(editor.addObjectProperty(0, "rotation", true));
    ASSERT_FALSE(editor.findChild<QGroupBox*>("sceneObjectRotationProperty")->isHidden());
    EXPECT_EQ(inspectorTabs->currentIndex(), 1);
    ASSERT_TRUE(editor.addObjectProperty(0, "asset", true));
    EXPECT_EQ(inspectorTabs->currentIndex(), 2);
    const SceneConfig finalScene = editor.sceneConfiguration();
    EXPECT_NE(std::find(finalScene.objects[0].properties.begin(),
                        finalScene.objects[0].properties.end(), "mirror"),
              finalScene.objects[0].properties.end());
    EXPECT_NE(std::find(finalScene.objects[0].properties.begin(),
                        finalScene.objects[0].properties.end(), "rotation"),
              finalScene.objects[0].properties.end());
    editor.close();
}
} // namespace bltzr_test_qt_scene_editor
