/*
 * @file modules/qt/window/scene/GuiSceneEditorState.cpp
 * @brief Scene object model synchronization and property actions.
 */

#include "window/scene/GuiSceneEditor.hpp"
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QWidget>
#include <algorithm>
#include <climits>
#include <limits>

namespace bltzr_qt {
SceneObjectConfig SceneEditor::legacyObject(const SimulationConfig& config) const
{
    SceneObjectConfig object;
    object.name = "Main scene object";
    const std::string preset = config.presetStructure;
    object.type = preset == "disk_orbit" ? "circle"
                  : preset == "random_cloud" || preset == "cube_random" || preset == "sphere_random"
                      ? "cloud"
                  : preset == "plummer_sphere"                      ? "plummer_sphere"
                  : preset == "galaxy_collision"                    ? "galaxy"
                  : preset == "two_body" || preset == "binary_star" ? "binary_star"
                  : preset == "sph_collapse"                        ? "cloud"
                                                                    : preset;
    object.particleCount = config.particleCount;
    object.seed = config.initSeed;
    object.mass = config.initDiskMass;
    object.size = config.initCloudHalfExtent;
    object.radiusMin = config.initDiskRadiusMin;
    object.radiusMax = config.initDiskRadiusMax;
    object.thickness = config.initDiskThickness;
    object.velocityScale = config.initVelocityScale;
    object.speed = config.initCloudSpeed;
    object.particleMass = config.initParticleMass;
    object.includeCentralBody = config.initIncludeCentralBody;
    object.positionX = config.initCentralX;
    object.positionY = config.initCentralY;
    object.positionZ = config.initCentralZ;
    object.velocityX = config.initCentralVx;
    object.velocityY = config.initCentralVy;
    object.velocityZ = config.initCentralVz;
    return object;
}

void SceneEditor::commitCurrent()
{
    if (_currentIndex < 0 || _currentIndex >= static_cast<int>(_scene.objects.size()))
        return;
    SceneObjectConfig& object = _scene.objects[static_cast<std::size_t>(_currentIndex)];
    object.name = _name->text().toStdString();
    object.type = _type->currentText().toStdString();
    object.enabled = _enabled->isChecked();
    object.includeCentralBody = _includeCentralBody->isChecked();
    object.particleCount = static_cast<std::uint32_t>(_particleCount->value());
    object.seed = static_cast<std::uint32_t>(_seed->value());
    object.mass = static_cast<float>(_mass->value());
    object.size = static_cast<float>(_size->value());
    object.radiusMin = static_cast<float>(_radiusMin->value());
    object.radiusMax = static_cast<float>(_radiusMax->value());
    object.thickness = static_cast<float>(_thickness->value());
    object.velocityScale = static_cast<float>(_velocityScale->value());
    object.speed = static_cast<float>(_speed->value());
    object.particleMass = static_cast<float>(_particleMass->value());
    if (object.type == "particle_system" && !hasProperty(object, "particle_system"))
        object.properties.push_back("particle_system");
    object.isAsset = _asset->isChecked();
    object.positionX = static_cast<float>(_positionX->value());
    object.positionY = static_cast<float>(_positionY->value());
    object.positionZ = static_cast<float>(_positionZ->value());
    object.velocityX = static_cast<float>(_velocityX->value());
    object.velocityY = static_cast<float>(_velocityY->value());
    object.velocityZ = static_cast<float>(_velocityZ->value());
    object.offsetX = static_cast<float>(_offsetX->value());
    object.offsetY = static_cast<float>(_offsetY->value());
    object.offsetZ = static_cast<float>(_offsetZ->value());
    object.rotationX = static_cast<float>(_rotationX->value());
    object.rotationY = static_cast<float>(_rotationY->value());
    object.rotationZ = static_cast<float>(_rotationZ->value());
    object.axis = _copyAxis->currentText().toStdString();
    object.copies = static_cast<std::uint32_t>(_rotationCopies->value());
    object.mirrorX = _mirrorX->isChecked();
    object.mirrorY = _mirrorY->isChecked();
    object.mirrorZ = _mirrorZ->isChecked();
    object.pivot = _pivot->currentText().toStdString();
    object.pivotX = static_cast<float>(_pivotX->value());
    object.pivotY = static_cast<float>(_pivotY->value());
    object.pivotZ = static_cast<float>(_pivotZ->value());
    if (object.type == "particle_system" || hasProperty(object, "particle_system")) {
        object.particleCount = static_cast<std::uint32_t>(_systemParticleCount->value());
        object.seed = static_cast<std::uint32_t>(_systemSeed->value());
        object.distribution = _distribution->currentText().toStdString();
        object.particleSize = static_cast<float>(_systemParticleSize->value());
        object.particleHeight = static_cast<float>(_systemParticleHeight->value());
        object.particleMass = static_cast<float>(_systemParticleMass->value());
        object.particleSpeed = static_cast<float>(_systemParticleSpeed->value());
        object.emitterObjectId = _emitterObject->currentData().toString().toStdString();
        object.targetAssetId = _targetAsset->currentData().toString().toStdString();
    }
    if (_objects->currentItem() != nullptr) {
        _objects->currentItem()->setText(QString::fromStdString(object.name));
        _objects->currentItem()->setToolTip(
            QString("%1 | %2 particles%3")
                .arg(QString::fromStdString(object.type))
                .arg(object.particleCount)
                .arg(object.properties.empty()
                         ? QString()
                         : QString(" | %1 properties").arg(object.properties.size())));
    }
}

void SceneEditor::loadCurrent()
{
    if (_currentIndex < 0 || _currentIndex >= static_cast<int>(_scene.objects.size())) {
        _name->setEnabled(false);
        _type->setEnabled(false);
        if (_propertyButton != nullptr)
            _propertyButton->setEnabled(false);
        if (_selectionLabel != nullptr)
            _selectionLabel->setText("No object selected");
        if (_inspectorMeta != nullptr)
            _inspectorMeta->clear();
        return;
    }
    _name->setEnabled(true);
    _type->setEnabled(true);
    if (_propertyButton != nullptr)
        _propertyButton->setEnabled(true);
    QWidget* fields[] = {_name.data(),
                         _type.data(),
                         _enabled.data(),
                         _includeCentralBody.data(),
                         _particleCount.data(),
                         _seed.data(),
                         _mass.data(),
                         _size.data(),
                         _radiusMin.data(),
                         _radiusMax.data(),
                         _thickness.data(),
                         _velocityScale.data(),
                         _speed.data(),
                         _particleMass.data(),
                         _asset.data(),
                         _positionX.data(),
                         _positionY.data(),
                         _positionZ.data(),
                         _velocityX.data(),
                         _velocityY.data(),
                         _velocityZ.data(),
                         _offsetX.data(),
                         _offsetY.data(),
                         _offsetZ.data(),
                         _rotationX.data(),
                         _rotationY.data(),
                         _rotationZ.data(),
                         _copyAxis.data(),
                         _rotationCopies.data(),
                         _mirrorX.data(),
                         _mirrorY.data(),
                         _mirrorZ.data(),
                         _pivot.data(),
                         _pivotX.data(),
                         _pivotY.data(),
                         _pivotZ.data(),
                         _emitterObject.data(),
                         _targetAsset.data(),
                         _distribution.data(),
                         _systemParticleCount.data(),
                         _systemSeed.data(),
                         _systemParticleSize.data(),
                         _systemParticleHeight.data(),
                         _systemParticleMass.data(),
                         _systemParticleSpeed.data()};
    for (QWidget* field : fields)
        field->blockSignals(true);
    const SceneObjectConfig& object = _scene.objects[static_cast<std::size_t>(_currentIndex)];
    if (_selectionLabel != nullptr)
        _selectionLabel->setText(QString("Editing %1").arg(QString::fromStdString(object.name)));
    if (_inspectorMeta != nullptr) {
        _inspectorMeta->setText(QString("%1  |  %2 particles  |  %3")
                                    .arg(QString::fromStdString(object.type))
                                    .arg(object.particleCount)
                                    .arg(object.enabled ? "enabled" : "disabled"));
    }
    if (auto* inspectorSelection = findChild<QLabel*>("sceneObjectInspectorSelection"))
        inspectorSelection->setText(QString::fromStdString(object.name));
    _name->setText(QString::fromStdString(object.name));
    _type->setCurrentText(QString::fromStdString(object.type));
    _enabled->setChecked(object.enabled);
    _includeCentralBody->setChecked(object.includeCentralBody);
    _particleCount->setValue(
        static_cast<int>(std::min<std::uint32_t>(object.particleCount, INT_MAX)));
    _seed->setValue(static_cast<int>(std::min<std::uint32_t>(object.seed, INT_MAX)));
    _mass->setValue(object.mass);
    _size->setValue(object.size);
    _radiusMin->setValue(object.radiusMin);
    _radiusMax->setValue(object.radiusMax);
    _thickness->setValue(object.thickness);
    _velocityScale->setValue(object.velocityScale);
    _speed->setValue(object.speed);
    _particleMass->setValue(object.particleMass);
    _asset->setChecked(object.isAsset);
    _positionX->setValue(object.positionX);
    _positionY->setValue(object.positionY);
    _positionZ->setValue(object.positionZ);
    _velocityX->setValue(object.velocityX);
    _velocityY->setValue(object.velocityY);
    _velocityZ->setValue(object.velocityZ);
    _offsetX->setValue(object.offsetX);
    _offsetY->setValue(object.offsetY);
    _offsetZ->setValue(object.offsetZ);
    _rotationX->setValue(object.rotationX);
    _rotationY->setValue(object.rotationY);
    _rotationZ->setValue(object.rotationZ);
    _copyAxis->setCurrentText(QString::fromStdString(object.axis));
    _rotationCopies->setValue(static_cast<int>(std::clamp(object.copies, 1u, 256u)));
    _mirrorX->setChecked(object.mirrorX);
    _mirrorY->setChecked(object.mirrorY);
    _mirrorZ->setChecked(object.mirrorZ);
    _pivot->setCurrentText(QString::fromStdString(object.pivot));
    _pivotX->setValue(object.pivotX);
    _pivotY->setValue(object.pivotY);
    _pivotZ->setValue(object.pivotZ);
    rebuildReferences();
    _emitterObject->setCurrentIndex(
        _emitterObject->findData(QString::fromStdString(object.emitterObjectId)));
    _targetAsset->setCurrentIndex(
        _targetAsset->findData(QString::fromStdString(object.targetAssetId)));
    _distribution->setCurrentText(QString::fromStdString(object.distribution));
    _systemParticleCount->setValue(static_cast<int>(std::min<std::uint32_t>(
        object.particleCount, static_cast<std::uint32_t>(std::numeric_limits<int>::max()))));
    _systemSeed->setValue(static_cast<int>(std::min<std::uint32_t>(
        object.seed, static_cast<std::uint32_t>(std::numeric_limits<int>::max()))));
    _systemParticleSize->setValue(object.particleSize);
    _systemParticleHeight->setValue(object.particleHeight);
    _systemParticleMass->setValue(object.particleMass);
    _systemParticleSpeed->setValue(object.particleSpeed);
    for (QWidget* field : fields)
        field->blockSignals(false);
    updateFieldVisibility();
}

void SceneEditor::rebuildList()
{
    _objects->blockSignals(true);
    _objects->clear();
    if (_objectCountLabel != nullptr)
        _objectCountLabel->setText(QString("%1 object%2")
                                       .arg(_scene.objects.size())
                                       .arg(_scene.objects.size() == 1u ? "" : "s"));
    for (std::size_t index = 0u; index < _scene.objects.size(); ++index) {
        const SceneObjectConfig& object = _scene.objects[index];
        auto* item = new QListWidgetItem(_objects);
        item->setSizeHint(QSize(0, 34));
        item->setText(QString::fromStdString(object.name));
        item->setToolTip(QString("%1 | %2 particles%3")
                             .arg(QString::fromStdString(object.type))
                             .arg(object.particleCount)
                             .arg(object.properties.empty()
                                      ? QString()
                                      : QString(" | %1 properties").arg(object.properties.size())));
    }
    _objects->blockSignals(false);
}

void SceneEditor::reload(const SimulationConfig& config)
{
    commitCurrent();
    _scene = config.scene;
    for (std::size_t index = 0u; index < _scene.objects.size(); ++index) {
        SceneObjectConfig& object = _scene.objects[index];
        if (object.id.empty())
            object.id = "object_" + std::to_string(index + 1);
        if (object.type == "plummer")
            object.type = "plummer_sphere";
    }
    if (_scene.objects.empty() && config.presetStructure != "file" &&
        config.initMode != "cosmology")
        _scene.objects.push_back(legacyObject(config));
    rebuildList();
    _currentIndex = _scene.objects.empty() ? -1 : 0;
    _objects->blockSignals(true);
    _objects->setCurrentRow(_currentIndex);
    _objects->blockSignals(false);
    loadCurrent();
}

} // namespace bltzr_qt
