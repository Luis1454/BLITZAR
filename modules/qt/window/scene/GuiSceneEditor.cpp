/*
 * @file modules/qt/window/scene/GuiSceneEditor.cpp
 * @brief Scene object editor construction and signal wiring.
 */

#include "window/scene/GuiSceneEditor.hpp"
#include <QAbstractItemView>
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
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>
#include <algorithm>

namespace bltzr_qt {
SceneObjectConfig defaultObject(int index)
{
    SceneObjectConfig object;
    object.id = "object_" + std::to_string(index + 1);
    object.name = "Object " + std::to_string(index + 1);
    object.seed += static_cast<std::uint32_t>(index);
    return object;
}

SceneEditor::SceneEditor(const SimulationConfig& config, QPointer<QWidget> parent) : QWidget(parent)
{
    buildLayout();
    connectObjectControls();
    reload(config);
}

void SceneEditor::buildLayout()
{
    QPointer<QVBoxLayout> root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    QPointer<QHBoxLayout> header = new QHBoxLayout();
    QPointer<QLabel> title = new QLabel("Scene objects", this);
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

    QPointer<QSplitter> split = new QSplitter(Qt::Vertical, this);
    split->setObjectName("sceneObjectEditorSplitter");
    split->setChildrenCollapsible(false);

    buildObjectPanel(split);

    buildInspectorPanel(split);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    split->setSizes({145, 620});
    root->addWidget(split, 1);
}

void SceneEditor::buildObjectPanel(QPointer<QSplitter> split)
{
    QPointer<QGroupBox> objectPanel = new QGroupBox("Objects", split);
    QPointer<QVBoxLayout> objectLayout = new QVBoxLayout(objectPanel);
    objectLayout->setContentsMargins(6, 6, 6, 6);
    objectLayout->setSpacing(4);

    QPointer<QHBoxLayout> objectHeader = new QHBoxLayout();
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

    QPointer<QHBoxLayout> buttons = new QHBoxLayout();
    _addObject = new QPushButton("Add", objectPanel);
    _addObject->setObjectName("sceneAddObjectButton");
    _addObject->setToolTip("Add a new scene object");
    _duplicateObject = new QPushButton("Duplicate", objectPanel);
    _duplicateObject->setObjectName("sceneDuplicateObjectButton");
    _duplicateObject->setToolTip("Duplicate the selected object");
    _removeObject = new QPushButton("Remove", objectPanel);
    _removeObject->setObjectName("sceneRemoveObjectButton");
    _removeObject->setToolTip("Remove the selected object");
    buttons->addWidget(_addObject);
    buttons->addWidget(_duplicateObject);
    buttons->addWidget(_removeObject);
    objectLayout->addLayout(buttons);
    split->addWidget(objectPanel);
}

void SceneEditor::buildInspectorPanel(QPointer<QSplitter> split)
{
    QPointer<QGroupBox> inspectorPanel = new QGroupBox("Object editor", split);
    QPointer<QVBoxLayout> inspectorLayout = new QVBoxLayout(inspectorPanel);
    inspectorLayout->setContentsMargins(6, 6, 6, 6);

    QPointer<QHBoxLayout> inspectorHeader = new QHBoxLayout();
    QPointer<QLabel> inspectorTitle = new QLabel("Object inspector", inspectorPanel);
    inspectorTitle->setObjectName("sceneObjectInspectorTitle");
    inspectorTitle->setStyleSheet("font-weight: 700; font-size: 14px;");
    inspectorHeader->addWidget(inspectorTitle);
    _inspectorSelection = new QLabel(inspectorPanel);
    _inspectorSelection->setObjectName("sceneObjectInspectorSelection");
    _inspectorSelection->setStyleSheet("font-weight: 600; color: #687080;");
    inspectorHeader->addWidget(_inspectorSelection, 1);
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
    _inspectorTabs = new QTabWidget(inspectorPanel);
    _inspectorTabs->setObjectName("sceneObjectInspector");
    _inspectorTabs->setDocumentMode(true);
    const auto addInspectorTab = [this](const QString& title, QPointer<QFormLayout>& form) {
        QPointer<QScrollArea> scroll = new QScrollArea(_inspectorTabs);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        QPointer<QWidget> formWidget = new QWidget(scroll);
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
}

void SceneEditor::connectObjectControls()
{
    connectCollectionControls();
    connectGeneratorControls();
    connectTransformControls();
    connectParticleSystemControls();
}

void SceneEditor::connectCollectionControls()
{
    connect(_propertyButton, &QPushButton::clicked, this, [this] {
        showPropertyMenu(_currentIndex, _propertyButton);
    });
    connect(_objects, &QListWidget::currentRowChanged, this, [this](int row) {
        commitCurrent();
        _currentIndex = row;
        loadCurrent();
    });
    connect(_addObject, &QPushButton::clicked, this, [this] {
        commitCurrent();
        _scene.objects.push_back(defaultObject(static_cast<int>(_scene.objects.size())));
        rebuildList();
        _objects->setCurrentRow(static_cast<int>(_scene.objects.size()) - 1);
    });
    connect(_duplicateObject, &QPushButton::clicked, this, [this] {
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
    connect(_removeObject, &QPushButton::clicked, this, [this] {
        commitCurrent();
        if (_currentIndex < 0 || _currentIndex >= static_cast<int>(_scene.objects.size()))
            return;
        _scene.objects.erase(_scene.objects.begin() + _currentIndex);
        rebuildList();
        _objects->setCurrentRow(std::min(_currentIndex, _objects->count() - 1));
    });
}

void SceneEditor::connectGeneratorControls()
{
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
    const QPointer<QDoubleSpinBox> fields[] = {_mass,      _size,        _radiusMin,
                                               _radiusMax, _thickness,   _velocityScale,
                                               _speed,     _particleMass};
    for (const QPointer<QDoubleSpinBox>& field : fields)
        connect(field, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
            commitCurrent();
        });
    connect(_asset, &QCheckBox::toggled, this, [this](bool) {
        commitCurrent();
    });
}

void SceneEditor::connectTransformControls()
{
    const QPointer<QDoubleSpinBox> objectFields[] = {_positionX,
                                                      _positionY,
                                                      _positionZ,
                                                      _velocityX,
                                                      _velocityY,
                                                      _velocityZ,
                                                      _offsetX,
                                                      _offsetY,
                                                      _offsetZ,
                                                      _rotationX,
                                                      _rotationY,
                                                      _rotationZ,
                                                      _pivotX,
                                                      _pivotY,
                                                      _pivotZ,
                                                      _systemParticleSize,
                                                      _systemParticleHeight,
                                                      _systemParticleMass,
                                                      _systemParticleSpeed};
    for (const QPointer<QDoubleSpinBox>& field : objectFields)
        connect(field, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
            commitCurrent();
        });
    const QPointer<QCheckBox> mirrorFields[] = {_mirrorX, _mirrorY, _mirrorZ};
    for (const QPointer<QCheckBox>& field : mirrorFields)
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
}

void SceneEditor::connectParticleSystemControls()
{
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

QPointer<QPushButton> SceneEditor::applyButton() const
{
    return _apply;
}
} // namespace bltzr_qt
