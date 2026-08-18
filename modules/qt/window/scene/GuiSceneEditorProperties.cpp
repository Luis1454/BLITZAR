/*
 * @file modules/qt/window/scene/GuiSceneEditorProperties.cpp
 * @brief Scene object property menu and reference synchronization.
 */

#include "window/scene/GuiSceneEditor.hpp"
#include <QAction>
#include <QComboBox>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QTabWidget>
#include <algorithm>

namespace bltzr_qt {
bool SceneEditor::hasProperty(const SceneObjectConfig& object, const std::string& property) const
{
    return std::find(object.properties.begin(), object.properties.end(), property) !=
           object.properties.end();
}

void SceneEditor::showPropertyMenu(int index, QPointer<QPushButton> anchor)
{
    if (index < 0 || index >= static_cast<int>(_scene.objects.size()))
        return;
    QMenu menu(this);
    const SceneObjectConfig& object = _scene.objects[static_cast<std::size_t>(index)];
    const std::pair<QString, QString> entries[] = {{"Offset", "offset"},
                                                   {"Rotation", "rotation"},
                                                   {"Copies", "copies"},
                                                   {"Mirror", "mirror"},
                                                   {"Pivot", "pivot"},
                                                   {"Asset", "asset"},
                                                   {"Particle system", "particle_system"}};
    for (const auto& entry : entries) {
        const bool present = hasProperty(object, entry.second);
        QPointer<QAction> action = menu.addAction(
            QString(present ? "Remove %1" : "Add %1").arg(entry.first));
        connect(action, &QAction::triggered, this, [this, index, entry, present] {
            addObjectProperty(index, entry.second.toStdString(), !present);
        });
    }
    menu.exec(anchor->mapToGlobal(QPoint(0, anchor->height())));
}

bool SceneEditor::addObjectProperty(int index, const std::string& property, bool enabled)
{
    if (index < 0 || index >= static_cast<int>(_scene.objects.size()))
        return false;
    commitCurrent();
    SceneObjectConfig& object = _scene.objects[static_cast<std::size_t>(index)];
    const auto position = std::find(object.properties.begin(), object.properties.end(), property);
    if (enabled) {
        if (position == object.properties.end())
            object.properties.push_back(property);
        if (property == "asset")
            object.isAsset = true;
    }
    else if (position != object.properties.end()) {
        object.properties.erase(position);
        if (property == "asset")
            object.isAsset = false;
        if (property == "offset")
            object.offsetX = object.offsetY = object.offsetZ = 0.0f;
        if (property == "rotation")
            object.rotationX = object.rotationY = object.rotationZ = 0.0f;
        if (property == "copies") {
            object.axis = "z";
            object.copies = 1u;
        }
        if (property == "mirror")
            object.mirrorX = object.mirrorY = object.mirrorZ = false;
        if (property == "pivot") {
            object.pivot = "world";
            object.pivotX = object.pivotY = object.pivotZ = 0.0f;
        }
    }
    rebuildList();
    _objects->setCurrentRow(index);
    loadCurrent();
    if (_inspectorTabs != nullptr) {
        const bool transform = property == "offset" || property == "rotation" ||
                               property == "copies" || property == "mirror" || property == "pivot";
        _inspectorTabs->setCurrentIndex(transform ? 1 : 2);
    }
    return hasProperty(_scene.objects[static_cast<std::size_t>(index)], property) == enabled;
}

void SceneEditor::rebuildReferences()
{
    _emitterObject->clear();
    _targetAsset->clear();
    for (const SceneObjectConfig& object : _scene.objects) {
        const QString label = QString::fromStdString(object.name);
        const QString id = QString::fromStdString(object.id);
        _emitterObject->addItem(label, id);
        if (object.isAsset || !object.enabled || object.type != "particle_system")
            _targetAsset->addItem(label, id);
    }
}
} // namespace bltzr_qt
