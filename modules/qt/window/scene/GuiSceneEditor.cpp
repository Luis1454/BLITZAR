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
    const auto addInspectorTab = [this](const QString& title, QPointer<QFormLayout>& form) {
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
    for (QDoubleSpinBox* field : fields)
        connect(field, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
            commitCurrent();
        });
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
    for (QDoubleSpinBox* field : objectFields)
        connect(field, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
            commitCurrent();
        });
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
