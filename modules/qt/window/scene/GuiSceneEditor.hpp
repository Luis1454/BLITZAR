/*
 * @file modules/qt/window/scene/GuiSceneEditor.hpp
 * @brief Scene object collection editor.
 */

#ifndef BLITZAR_MODULES_QT_SRC_WINDOW_SCENE_SCENEEDITOR_HPP_
#define BLITZAR_MODULES_QT_SRC_WINDOW_SCENE_SCENEEDITOR_HPP_

#include "config/core/configuration/CfgConfig.hpp"
#include <QPointer>
#include <QWidget>

class QListWidget;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QFormLayout;
class QGroupBox;
class QLabel;
class QTabWidget;
class QSplitter;

namespace bltzr_qt {
class SceneEditor final : public QWidget {
public:
    explicit SceneEditor(const SimulationConfig& config, QPointer<QWidget> parent = nullptr);

    SceneConfig sceneConfiguration();
    void applyToConfig(SimulationConfig& config);
    void reload(const SimulationConfig& config);
    bool addObjectProperty(int index, const std::string& property, bool enabled);
    QPointer<QPushButton> applyButton() const;

private:
    void buildLayout();
    void buildObjectPanel(QPointer<QSplitter> split);
    void buildInspectorPanel(QPointer<QSplitter> split);
    void connectObjectControls();
    void connectCollectionControls();
    void connectGeneratorControls();
    void connectTransformControls();
    void connectParticleSystemControls();
    void commitCurrent();
    void loadCurrent();
    void rebuildList();
    void rebuildReferences();
    void showPropertyMenu(int index, QPointer<QPushButton> anchor);
    bool hasProperty(const SceneObjectConfig& object, const std::string& property) const;
    void updateFieldVisibility();
    SceneObjectConfig legacyObject(const SimulationConfig& config) const;
    void addObjectFields();

    SceneConfig _scene;
    int _currentIndex = -1;
    QPointer<QFormLayout> _generatorForm;
    QPointer<QFormLayout> _transformForm;
    QPointer<QFormLayout> _propertiesForm;
    QPointer<QListWidget> _objects;
    QPointer<QLabel> _objectCountLabel;
    QPointer<QLabel> _selectionLabel;
    QPointer<QLabel> _inspectorSelection;
    QPointer<QLabel> _inspectorMeta;
    QPointer<QLabel> _propertiesHint;
    QPointer<QTabWidget> _inspectorTabs;
    QPointer<QLineEdit> _name;
    QPointer<QComboBox> _type;
    QPointer<QCheckBox> _enabled;
    QPointer<QCheckBox> _includeCentralBody;
    QPointer<QSpinBox> _particleCount;
    QPointer<QSpinBox> _seed;
    QPointer<QDoubleSpinBox> _mass;
    QPointer<QDoubleSpinBox> _size;
    QPointer<QDoubleSpinBox> _radiusMin;
    QPointer<QDoubleSpinBox> _radiusMax;
    QPointer<QDoubleSpinBox> _thickness;
    QPointer<QDoubleSpinBox> _velocityScale;
    QPointer<QDoubleSpinBox> _speed;
    QPointer<QDoubleSpinBox> _particleMass;
    QPointer<QCheckBox> _asset;
    QPointer<QDoubleSpinBox> _positionX;
    QPointer<QDoubleSpinBox> _positionY;
    QPointer<QDoubleSpinBox> _positionZ;
    QPointer<QDoubleSpinBox> _velocityX;
    QPointer<QDoubleSpinBox> _velocityY;
    QPointer<QDoubleSpinBox> _velocityZ;
    QPointer<QDoubleSpinBox> _offsetX;
    QPointer<QDoubleSpinBox> _offsetY;
    QPointer<QDoubleSpinBox> _offsetZ;
    QPointer<QDoubleSpinBox> _rotationX;
    QPointer<QDoubleSpinBox> _rotationY;
    QPointer<QDoubleSpinBox> _rotationZ;
    QPointer<QComboBox> _copyAxis;
    QPointer<QSpinBox> _rotationCopies;
    QPointer<QCheckBox> _mirrorX;
    QPointer<QCheckBox> _mirrorY;
    QPointer<QCheckBox> _mirrorZ;
    QPointer<QComboBox> _pivot;
    QPointer<QDoubleSpinBox> _pivotX;
    QPointer<QDoubleSpinBox> _pivotY;
    QPointer<QDoubleSpinBox> _pivotZ;
    QPointer<QGroupBox> _particleSystemGroup;
    QPointer<QComboBox> _emitterObject;
    QPointer<QComboBox> _targetAsset;
    QPointer<QComboBox> _distribution;
    QPointer<QSpinBox> _systemParticleCount;
    QPointer<QSpinBox> _systemSeed;
    QPointer<QDoubleSpinBox> _systemParticleSize;
    QPointer<QDoubleSpinBox> _systemParticleHeight;
    QPointer<QDoubleSpinBox> _systemParticleMass;
    QPointer<QDoubleSpinBox> _systemParticleSpeed;
    QPointer<QFormLayout> _baseForm;
    QPointer<QFormLayout> _pivotForm;
    QPointer<QGroupBox> _assetGroup;
    QPointer<QGroupBox> _offsetGroup;
    QPointer<QGroupBox> _rotationGroup;
    QPointer<QGroupBox> _copiesGroup;
    QPointer<QGroupBox> _mirrorGroup;
    QPointer<QGroupBox> _pivotGroup;
    QPointer<QPushButton> _propertyButton;
    QPointer<QPushButton> _addObject;
    QPointer<QPushButton> _duplicateObject;
    QPointer<QPushButton> _removeObject;
    QPointer<QPushButton> _apply;
};
} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_SRC_WINDOW_SCENE_SCENEEDITOR_HPP_
