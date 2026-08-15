/*
 * @file modules/qt/src/window/scene/SceneEditor.cpp
 * @brief Scene object collection editor implementation.
 */

#include "window/scene/SceneEditor.hpp"
#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <climits>
#include <limits>

namespace bltzr_qt {
namespace {
QDoubleSpinBox* floatField(QFormLayout* form, const char* label, double value,
                           double minimum = -1.0e9, double maximum = 1.0e9)
{
    auto* field = new QDoubleSpinBox(form->parentWidget());
    field->setObjectName(label);
    field->setDecimals(4);
    field->setRange(minimum, maximum);
    field->setSingleStep(0.1);
    field->setValue(value);
    form->addRow(label, field);
    return field;
}

SceneObjectConfig defaultObject(int index)
{
    SceneObjectConfig object;
    object.id = "object_" + std::to_string(index + 1);
    object.name = "Object " + std::to_string(index + 1);
    object.seed += static_cast<std::uint32_t>(index);
    return object;
}
} // namespace

SceneEditor::SceneEditor(const SimulationConfig& config, QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    auto* header = new QHBoxLayout();
    auto* title = new QLabel("Scene objects", this);
    title->setStyleSheet("font-weight: 700; font-size: 15px;");
    header->addWidget(title);
    _selectionLabel = new QLabel(this);
    _selectionLabel->setObjectName("sceneSelectionLabel");
    _selectionLabel->setStyleSheet("color: #687080;");
    header->addWidget(_selectionLabel, 1);
    _apply = new QPushButton("Apply Scene", this);
    _apply->setObjectName("applySceneObjectsButton");
    _apply->setToolTip("Send the current scene objects to the simulation");
    header->addWidget(_apply, 0);
    root->addLayout(header);

    auto* split = new QSplitter(Qt::Vertical, this);
    split->setObjectName("sceneObjectEditorSplitter");
    split->setChildrenCollapsible(false);

    auto* objectPanel = new QGroupBox("Objects", split);
    auto* objectLayout = new QVBoxLayout(objectPanel);
    objectLayout->setContentsMargins(6, 6, 6, 6);
    objectLayout->setSpacing(4);
    auto* objectHeader = new QHBoxLayout();
    _objectCountLabel = new QLabel(objectPanel);
    _objectCountLabel->setObjectName("sceneObjectCountLabel");
    _objectCountLabel->setStyleSheet("color: #687080;");
    objectHeader->addWidget(_objectCountLabel);
    objectHeader->addStretch(1);
    objectLayout->addLayout(objectHeader);
    _objects = new QListWidget(objectPanel);
    _objects->setObjectName("sceneObjectList");
    _objects->setMinimumHeight(100);
    _objects->setAlternatingRowColors(true);
    _objects->setUniformItemSizes(true);
    _objects->setSelectionMode(QAbstractItemView::SingleSelection);
    objectLayout->addWidget(_objects, 1);
    auto* buttons = new QHBoxLayout();
    auto* add = new QPushButton("Add", objectPanel);
    add->setObjectName("sceneAddObjectButton");
    add->setToolTip("Add a new scene object");
    auto* duplicate = new QPushButton("Duplicate", objectPanel);
    duplicate->setObjectName("sceneDuplicateObjectButton");
    duplicate->setToolTip("Duplicate the selected object");
    auto* remove = new QPushButton("Remove", objectPanel);
    remove->setObjectName("sceneRemoveObjectButton");
    remove->setToolTip("Remove the selected object");
    buttons->addWidget(add);
    buttons->addWidget(duplicate);
    buttons->addWidget(remove);
    objectLayout->addLayout(buttons);
    split->addWidget(objectPanel);

    auto* inspectorPanel = new QGroupBox("Object editor", split);
    auto* inspectorLayout = new QVBoxLayout(inspectorPanel);
    inspectorLayout->setContentsMargins(6, 6, 6, 6);
    auto* inspectorHeader = new QHBoxLayout();
    auto* inspectorTitle = new QLabel("Object inspector", inspectorPanel);
    inspectorTitle->setObjectName("sceneObjectInspectorTitle");
    inspectorTitle->setStyleSheet("font-weight: 700; font-size: 14px;");
    inspectorHeader->addWidget(inspectorTitle);
    auto* inspectorSelection = new QLabel(inspectorPanel);
    inspectorSelection->setObjectName("sceneObjectInspectorSelection");
    inspectorSelection->setStyleSheet("font-weight: 600; color: #687080;");
    inspectorHeader->addWidget(inspectorSelection, 1);
    _propertyButton = new QPushButton("+ Add property", inspectorPanel);
    _propertyButton->setObjectName("sceneObjectPropertyButton");
    _propertyButton->setAccessibleName("Add object property");
    _propertyButton->setToolTip("Add an independent property to the selected object");
    inspectorHeader->addWidget(_propertyButton, 0);
    inspectorLayout->addLayout(inspectorHeader);
    _inspectorMeta = new QLabel(inspectorPanel);
    _inspectorMeta->setObjectName("sceneObjectInspectorMeta");
    _inspectorMeta->setStyleSheet("color: #687080; padding-bottom: 3px;");
    inspectorLayout->addWidget(_inspectorMeta);
    connect(_propertyButton, &QPushButton::clicked, this, [this] {
        showPropertyMenu(_currentIndex, _propertyButton);
    });
    _inspectorTabs = new QTabWidget(inspectorPanel);
    _inspectorTabs->setObjectName("sceneObjectInspector");
    _inspectorTabs->setDocumentMode(true);
    const auto addInspectorTab = [this](const QString& title, QFormLayout*& form) {
        auto* scroll = new QScrollArea(_inspectorTabs);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        auto* formWidget = new QWidget(scroll);
        form = new QFormLayout(formWidget);
        form->setContentsMargins(2, 4, 8, 4);
        form->setVerticalSpacing(7);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        scroll->setWidget(formWidget);
        _inspectorTabs->addTab(scroll, title);
    };
    addInspectorTab("Generator", _generatorForm);
    addInspectorTab("Transform", _transformForm);
    addInspectorTab("Properties", _propertiesForm);
    addObjectFields();
    inspectorLayout->addWidget(_inspectorTabs, 1);
    split->addWidget(inspectorPanel);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    split->setSizes({145, 620});
    root->addWidget(split, 1);

    connect(_objects, &QListWidget::currentRowChanged, this, [this](int row) {
        commitCurrent();
        _currentIndex = row;
        loadCurrent();
    });
    connect(add, &QPushButton::clicked, this, [this] {
        commitCurrent();
        _scene.objects.push_back(defaultObject(static_cast<int>(_scene.objects.size())));
        rebuildList();
        _objects->setCurrentRow(static_cast<int>(_scene.objects.size()) - 1);
    });
    connect(duplicate, &QPushButton::clicked, this, [this] {
        commitCurrent();
        if (_currentIndex < 0 || _currentIndex >= static_cast<int>(_scene.objects.size()))
            return;
        SceneObjectConfig copy = _scene.objects[static_cast<std::size_t>(_currentIndex)];
        copy.id = "object_" + std::to_string(_scene.objects.size() + 1);
        copy.name += " Copy";
        copy.seed += 1u;
        _scene.objects.push_back(std::move(copy));
        rebuildList();
        _objects->setCurrentRow(static_cast<int>(_scene.objects.size()) - 1);
    });
    connect(remove, &QPushButton::clicked, this, [this] {
        commitCurrent();
        if (_currentIndex < 0 || _currentIndex >= static_cast<int>(_scene.objects.size()))
            return;
        _scene.objects.erase(_scene.objects.begin() + _currentIndex);
        rebuildList();
        _objects->setCurrentRow(std::min(_currentIndex, _objects->count() - 1));
    });
    const auto commit = [this] {
        commitCurrent();
    };
    connect(_name, &QLineEdit::editingFinished, this, commit);
    connect(_type, &QComboBox::currentTextChanged, this, [this](const QString&) {
        commitCurrent();
        updateFieldVisibility();
    });
    connect(_enabled, &QCheckBox::toggled, this, [this](bool) {
        commitCurrent();
    });
    connect(_includeCentralBody, &QCheckBox::toggled, this, [this](bool) {
        commitCurrent();
    });
    connect(_particleCount, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        commitCurrent();
    });
    connect(_seed, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        commitCurrent();
    });
    QDoubleSpinBox* fields[] = {_mass.data(),      _size.data(),        _radiusMin.data(),
                                _radiusMax.data(), _thickness.data(),   _velocityScale.data(),
                                _speed.data(),     _particleMass.data()};
    for (QDoubleSpinBox* field : fields) {
        connect(field, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
            commitCurrent();
        });
    }
    connect(_asset, &QCheckBox::toggled, this, [this](bool) {
        commitCurrent();
    });
    QDoubleSpinBox* objectFields[] = {_positionX.data(),
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
                                      _pivotX.data(),
                                      _pivotY.data(),
                                      _pivotZ.data(),
                                      _systemParticleSize.data(),
                                      _systemParticleHeight.data(),
                                      _systemParticleMass.data(),
                                      _systemParticleSpeed.data()};
    for (QDoubleSpinBox* field : objectFields) {
        connect(field, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
            commitCurrent();
        });
    }
    QCheckBox* mirrorFields[] = {_mirrorX.data(), _mirrorY.data(), _mirrorZ.data()};
    for (QCheckBox* field : mirrorFields)
        connect(field, &QCheckBox::toggled, this, [this](bool) {
            commitCurrent();
        });
    connect(_pivot, &QComboBox::currentTextChanged, this, [this](const QString&) {
        updateFieldVisibility();
        commitCurrent();
    });
    connect(_copyAxis, &QComboBox::currentTextChanged, this, [this](const QString&) {
        commitCurrent();
    });
    connect(_rotationCopies, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        commitCurrent();
    });
    connect(_emitterObject, &QComboBox::currentTextChanged, this, [this](const QString&) {
        commitCurrent();
    });
    connect(_targetAsset, &QComboBox::currentTextChanged, this, [this](const QString&) {
        commitCurrent();
    });
    connect(_distribution, &QComboBox::currentTextChanged, this, [this](const QString&) {
        commitCurrent();
    });
    connect(_systemParticleCount, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        commitCurrent();
    });
    connect(_systemSeed, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        commitCurrent();
    });
    reload(config);
}

void SceneEditor::addObjectFields()
{
    QWidget* generatorParent = _generatorForm->parentWidget();
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
    auto* assetForm = new QFormLayout(_assetGroup);
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
    auto* base = new QGroupBox("Position and velocity", _transformForm->parentWidget());
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
    auto* offsetForm = new QFormLayout(_offsetGroup);
    _offsetX = floatField(offsetForm, "Offset X", 0.0);
    _offsetY = floatField(offsetForm, "Offset Y", 0.0);
    _offsetZ = floatField(offsetForm, "Offset Z", 0.0);
    _transformForm->addRow(_offsetGroup);
    _rotationGroup = new QGroupBox("Rotation property", _transformForm->parentWidget());
    _rotationGroup->setObjectName("sceneObjectRotationProperty");
    auto* rotationForm = new QFormLayout(_rotationGroup);
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
    auto* copiesForm = new QFormLayout(_copiesGroup);
    _copyAxis = new QComboBox(_copiesGroup);
    _copyAxis->addItems({"x", "y", "z"});
    _rotationCopies = new QSpinBox(_copiesGroup);
    _rotationCopies->setRange(1, 256);
    copiesForm->addRow("Copy axis", _copyAxis);
    copiesForm->addRow("Copies", _rotationCopies);
    _transformForm->addRow(_copiesGroup);
    _mirrorGroup = new QGroupBox("Mirror property", _transformForm->parentWidget());
    _mirrorGroup->setObjectName("sceneObjectMirrorProperty");
    auto* mirrorForm = new QFormLayout(_mirrorGroup);
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
    auto* systemForm = new QFormLayout(_particleSystemGroup);
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
    const SceneObjectConfig* object = nullptr;
    if (_currentIndex >= 0 && _currentIndex < static_cast<int>(_scene.objects.size()))
        object = &_scene.objects[static_cast<std::size_t>(_currentIndex)];
    const bool particleSystem = object != nullptr && (object->type == "particle_system" ||
                                                      hasProperty(*object, "particle_system"));
    const bool generator = !particleSystem;
    QWidget* fields[] = {_includeCentralBody, _particleCount, _seed,      _mass,          _size,
                         _radiusMin,          _radiusMax,     _thickness, _velocityScale, _speed,
                         _particleMass};
    for (QWidget* field : fields) {
        if (field == nullptr)
            continue;
        if (QWidget* label = _generatorForm->labelForField(field))
            label->setVisible(generator);
        field->setVisible(generator);
    }
    const bool asset = object != nullptr && hasProperty(*object, "asset");
    if (_propertiesHint != nullptr)
        _propertiesHint->setVisible(!asset && !particleSystem);
    if (_assetGroup != nullptr)
        _assetGroup->setVisible(asset);
    if (_offsetGroup != nullptr)
        _offsetGroup->setVisible(object != nullptr && hasProperty(*object, "offset"));
    if (_rotationGroup != nullptr)
        _rotationGroup->setVisible(object != nullptr && hasProperty(*object, "rotation"));
    if (_copiesGroup != nullptr)
        _copiesGroup->setVisible(object != nullptr && hasProperty(*object, "copies"));
    if (_mirrorGroup != nullptr)
        _mirrorGroup->setVisible(object != nullptr && hasProperty(*object, "mirror"));
    if (_pivotGroup != nullptr)
        _pivotGroup->setVisible(object != nullptr && hasProperty(*object, "pivot"));
    if (_particleSystemGroup != nullptr)
        _particleSystemGroup->setVisible(particleSystem);
    const bool customPivot = _pivotGroup != nullptr && _pivotGroup->isVisible() &&
                             _pivot != nullptr && _pivot->currentText() == "custom";
    for (QWidget* field : {_pivotX.data(), _pivotY.data(), _pivotZ.data()}) {
        if (field == nullptr)
            continue;
        if (QWidget* label = _pivotForm->labelForField(field))
            label->setVisible(customPivot);
        field->setVisible(customPivot);
    }
}

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

bool SceneEditor::hasProperty(const SceneObjectConfig& object, const std::string& property) const
{
    return std::find(object.properties.begin(), object.properties.end(), property) !=
           object.properties.end();
}

void SceneEditor::showPropertyMenu(int index, QPushButton* anchor)
{
    if (index < 0 || index >= static_cast<int>(_scene.objects.size()))
        return;
    QMenu menu(this);
    const SceneObjectConfig& object = _scene.objects[static_cast<std::size_t>(index)];
    const std::pair<const char*, const char*> entries[] = {{"Offset", "offset"},
                                                           {"Rotation", "rotation"},
                                                           {"Copies", "copies"},
                                                           {"Mirror", "mirror"},
                                                           {"Pivot", "pivot"},
                                                           {"Asset", "asset"},
                                                           {"Particle system", "particle_system"}};
    for (const auto& entry : entries) {
        const bool present = hasProperty(object, entry.second);
        QAction* action = menu.addAction(
            QString(present ? "Remove %1" : "Add %1").arg(QString::fromUtf8(entry.first)));
        connect(action, &QAction::triggered, this, [this, index, entry, present] {
            addObjectProperty(index, entry.second, !present);
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
        if (property == "offset") {
            object.offsetX = object.offsetY = object.offsetZ = 0.0f;
        }
        if (property == "rotation") {
            object.rotationX = object.rotationY = object.rotationZ = 0.0f;
        }
        if (property == "copies") {
            object.axis = "z";
            object.copies = 1u;
        }
        if (property == "mirror") {
            object.mirrorX = object.mirrorY = object.mirrorZ = false;
        }
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

SceneConfig SceneEditor::sceneConfiguration()
{
    commitCurrent();
    return _scene;
}

void SceneEditor::applyToConfig(SimulationConfig& config)
{
    config.scene = sceneConfiguration();
}

QPushButton* SceneEditor::applyButton() const
{
    return _apply;
}
} // namespace bltzr_qt
