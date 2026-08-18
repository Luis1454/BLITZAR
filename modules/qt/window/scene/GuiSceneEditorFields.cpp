/*
 * @file modules/qt/window/scene/GuiSceneEditorFields.cpp
 * @brief Scene object generator and property field construction.
 */

#include "window/scene/GuiSceneEditor.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QWidget>
#include <limits>

namespace bltzr_qt {
static QPointer<QDoubleSpinBox> floatField(QPointer<QFormLayout> form, const QString& label,
                                           double value, double minimum = -1.0e9,
                                           double maximum = 1.0e9)
{
    QPointer<QDoubleSpinBox> field = new QDoubleSpinBox(form->parentWidget());
    field->setObjectName(label);
    field->setDecimals(4);
    field->setRange(minimum, maximum);
    field->setSingleStep(0.1);
    field->setValue(value);
    form->addRow(label, field);
    return field;
}

void SceneEditor::addObjectFields()
{
    QPointer<QWidget> generatorParent = _generatorForm->parentWidget();
    _generatorForm->setVerticalSpacing(5);
    _name = new QLineEdit(generatorParent);
    _generatorForm->addRow("Name", _name);
    _type = new QComboBox(generatorParent);
    _type->addItems({"point", "circle", "cloud", "plummer_sphere", "galaxy", "binary_star",
                     "solar_system", "particle_system"});
    _generatorForm->addRow("Generator", _type);
    _enabled = new QCheckBox(generatorParent);
    _generatorForm->addRow("Enabled", _enabled);
    _includeCentralBody = new QCheckBox(generatorParent);
    _generatorForm->addRow("Central body", _includeCentralBody);
    _particleCount = new QSpinBox(generatorParent);
    _particleCount->setRange(1, std::numeric_limits<int>::max());
    _generatorForm->addRow("Particle count", _particleCount);
    _seed = new QSpinBox(generatorParent);
    _seed->setRange(0, std::numeric_limits<int>::max());
    _generatorForm->addRow("Seed", _seed);
    _mass = floatField(_generatorForm, "Mass", 0.75, 0.000001, 1.0e15);
    _size = floatField(_generatorForm, "Size", 12.0, 0.0001, 1.0e9);
    _radiusMin = floatField(_generatorForm, "Radius min", 1.5, 0.0001, 1.0e9);
    _radiusMax = floatField(_generatorForm, "Radius max", 11.5, 0.0001, 1.0e9);
    _thickness = floatField(_generatorForm, "Thickness", 0.0, 0.0, 1.0e9);
    _velocityScale = floatField(_generatorForm, "Velocity scale", 1.0, 0.0, 1.0e6);
    _speed = floatField(_generatorForm, "Cloud speed", 0.0, 0.0, 1.0e6);
    _particleMass = floatField(_generatorForm, "Particle mass", 0.01, 0.000001, 1.0e15);

    _assetGroup = new QGroupBox("Asset property", _propertiesForm->parentWidget());
    QPointer<QFormLayout> assetForm = new QFormLayout(_assetGroup);
    _asset = new QCheckBox(_assetGroup);
    _asset->setObjectName("sceneObjectAssetCheck");
    assetForm->addRow("Publish as asset", _asset);
    _propertiesForm->addRow(_assetGroup);
    _propertiesHint = new QLabel(
        "No optional properties. Use + Add property for asset publishing or particle systems.",
        _propertiesForm->parentWidget());
    _propertiesHint->setWordWrap(true);
    _propertiesHint->setStyleSheet("color: #687080; padding: 8px;");
    _propertiesForm->addRow(_propertiesHint);

    QPointer<QGroupBox> base = new QGroupBox("Position and velocity", _transformForm->parentWidget());
    _baseForm = new QFormLayout(base);
    _positionX = floatField(_baseForm, "Position X", 0.0);
    _positionY = floatField(_baseForm, "Position Y", 0.0);
    _positionZ = floatField(_baseForm, "Position Z", 0.0);
    _velocityX = floatField(_baseForm, "Velocity X", 0.0);
    _velocityY = floatField(_baseForm, "Velocity Y", 0.0);
    _velocityZ = floatField(_baseForm, "Velocity Z", 0.0);
    _positionX->setObjectName("sceneObjectPositionX");
    _positionY->setObjectName("sceneObjectPositionY");
    _positionZ->setObjectName("sceneObjectPositionZ");
    _velocityX->setObjectName("sceneObjectVelocityX");
    _velocityY->setObjectName("sceneObjectVelocityY");
    _velocityZ->setObjectName("sceneObjectVelocityZ");
    _transformForm->addRow(base);

    _offsetGroup = new QGroupBox("Offset property", _transformForm->parentWidget());
    _offsetGroup->setObjectName("sceneObjectOffsetProperty");
    QPointer<QFormLayout> offsetForm = new QFormLayout(_offsetGroup);
    _offsetX = floatField(offsetForm, "Offset X", 0.0);
    _offsetY = floatField(offsetForm, "Offset Y", 0.0);
    _offsetZ = floatField(offsetForm, "Offset Z", 0.0);
    _transformForm->addRow(_offsetGroup);

    _rotationGroup = new QGroupBox("Rotation property", _transformForm->parentWidget());
    _rotationGroup->setObjectName("sceneObjectRotationProperty");
    QPointer<QFormLayout> rotationForm = new QFormLayout(_rotationGroup);
    _rotationX = floatField(rotationForm, "Rotation X", 0.0);
    _rotationY = floatField(rotationForm, "Rotation Y", 0.0);
    _rotationZ = floatField(rotationForm, "Rotation Z", 0.0);
    _transformForm->addRow(_rotationGroup);
    _offsetX->setObjectName("sceneObjectOffsetX");
    _offsetY->setObjectName("sceneObjectOffsetY");
    _offsetZ->setObjectName("sceneObjectOffsetZ");
    _rotationX->setObjectName("sceneObjectRotationX");
    _rotationY->setObjectName("sceneObjectRotationY");
    _rotationZ->setObjectName("sceneObjectRotationZ");

    _copiesGroup = new QGroupBox("Copies property", _transformForm->parentWidget());
    _copiesGroup->setObjectName("sceneObjectCopiesProperty");
    QPointer<QFormLayout> copiesForm = new QFormLayout(_copiesGroup);
    _copyAxis = new QComboBox(_copiesGroup);
    _copyAxis->addItems({"x", "y", "z"});
    _rotationCopies = new QSpinBox(_copiesGroup);
    _rotationCopies->setRange(1, 256);
    copiesForm->addRow("Copy axis", _copyAxis);
    copiesForm->addRow("Copies", _rotationCopies);
    _transformForm->addRow(_copiesGroup);

    _mirrorGroup = new QGroupBox("Mirror property", _transformForm->parentWidget());
    _mirrorGroup->setObjectName("sceneObjectMirrorProperty");
    QPointer<QFormLayout> mirrorForm = new QFormLayout(_mirrorGroup);
    _mirrorX = new QCheckBox(_mirrorGroup);
    _mirrorY = new QCheckBox(_mirrorGroup);
    _mirrorZ = new QCheckBox(_mirrorGroup);
    mirrorForm->addRow("Mirror X", _mirrorX);
    mirrorForm->addRow("Mirror Y", _mirrorY);
    mirrorForm->addRow("Mirror Z", _mirrorZ);
    _transformForm->addRow(_mirrorGroup);

    _pivotGroup = new QGroupBox("Pivot property", _transformForm->parentWidget());
    _pivotGroup->setObjectName("sceneObjectPivotProperty");
    _pivotForm = new QFormLayout(_pivotGroup);
    _pivot = new QComboBox(_pivotGroup);
    _pivot->addItems({"world", "object", "custom"});
    _pivotX = floatField(_pivotForm, "Pivot X", 0.0);
    _pivotY = floatField(_pivotForm, "Pivot Y", 0.0);
    _pivotZ = floatField(_pivotForm, "Pivot Z", 0.0);
    _pivotForm->addRow("Pivot", _pivot);
    _transformForm->addRow(_pivotGroup);

    _particleSystemGroup =
        new QGroupBox("Particle system properties", _propertiesForm->parentWidget());
    _particleSystemGroup->setObjectName("sceneObjectParticleSystemProperty");
    QPointer<QFormLayout> systemForm = new QFormLayout(_particleSystemGroup);
    _emitterObject = new QComboBox(_particleSystemGroup);
    _targetAsset = new QComboBox(_particleSystemGroup);
    _distribution = new QComboBox(_particleSystemGroup);
    _distribution->addItems({"uniform_sphere", "uniform_box", "forest", "object_particles"});
    _systemParticleCount = new QSpinBox(_particleSystemGroup);
    _systemParticleCount->setObjectName("sceneParticleSystemCount");
    _systemParticleCount->setRange(0, std::numeric_limits<int>::max());
    _systemSeed = new QSpinBox(_particleSystemGroup);
    _systemSeed->setObjectName("sceneParticleSystemSeed");
    _systemSeed->setRange(0, std::numeric_limits<int>::max());
    _systemParticleSize = floatField(systemForm, "Particle size", 1.0);
    _systemParticleHeight = floatField(systemForm, "Particle height", 1.0);
    _systemParticleMass = floatField(systemForm, "Particle mass", 0.001);
    _systemParticleSpeed = floatField(systemForm, "Particle speed", 0.0);
    systemForm->addRow("Emitter object", _emitterObject);
    systemForm->addRow("Target asset", _targetAsset);
    systemForm->addRow("Distribution", _distribution);
    systemForm->addRow("Particle count", _systemParticleCount);
    systemForm->addRow("Seed", _systemSeed);
    _propertiesForm->addRow(_particleSystemGroup);
    updateFieldVisibility();
}

void SceneEditor::updateFieldVisibility()
{
    const bool hasObject = _currentIndex >= 0 &&
                           _currentIndex < static_cast<int>(_scene.objects.size());
    const std::size_t objectIndex = hasObject ? static_cast<std::size_t>(_currentIndex) : 0u;
    const bool particleSystem = hasObject &&
                                (_scene.objects[objectIndex].type == "particle_system" ||
                                 hasProperty(_scene.objects[objectIndex], "particle_system"));
    const bool generator = !particleSystem;
    const QPointer<QWidget> fields[] = {_includeCentralBody, _particleCount, _seed, _mass, _size,
                                        _radiusMin, _radiusMax, _thickness, _velocityScale, _speed,
                                        _particleMass};
    for (const QPointer<QWidget>& field : fields) {
        if (field == nullptr)
            continue;
        const QPointer<QWidget> label = _generatorForm->labelForField(field);
        if (label != nullptr)
            label->setVisible(generator);
        field->setVisible(generator);
    }
    const bool asset = hasObject && hasProperty(_scene.objects[objectIndex], "asset");
    if (_propertiesHint != nullptr)
        _propertiesHint->setVisible(!asset && !particleSystem);
    if (_assetGroup != nullptr)
        _assetGroup->setVisible(asset);
    if (_offsetGroup != nullptr)
        _offsetGroup->setVisible(hasObject && hasProperty(_scene.objects[objectIndex], "offset"));
    if (_rotationGroup != nullptr)
        _rotationGroup->setVisible(hasObject && hasProperty(_scene.objects[objectIndex], "rotation"));
    if (_copiesGroup != nullptr)
        _copiesGroup->setVisible(hasObject && hasProperty(_scene.objects[objectIndex], "copies"));
    if (_mirrorGroup != nullptr)
        _mirrorGroup->setVisible(hasObject && hasProperty(_scene.objects[objectIndex], "mirror"));
    if (_pivotGroup != nullptr)
        _pivotGroup->setVisible(hasObject && hasProperty(_scene.objects[objectIndex], "pivot"));
    if (_particleSystemGroup != nullptr)
        _particleSystemGroup->setVisible(particleSystem);
    const bool customPivot = _pivotGroup != nullptr && _pivotGroup->isVisible() &&
                             _pivot != nullptr && _pivot->currentText() == "custom";
    for (const QPointer<QWidget>& field : {QPointer<QWidget>(_pivotX), QPointer<QWidget>(_pivotY),
                                           QPointer<QWidget>(_pivotZ)}) {
        if (field == nullptr)
            continue;
        const QPointer<QWidget> label = _pivotForm->labelForField(field);
        if (label != nullptr)
            label->setVisible(customPivot);
        field->setVisible(customPivot);
    }
}
} // namespace bltzr_qt
