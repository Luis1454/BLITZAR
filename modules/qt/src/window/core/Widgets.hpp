/*
 * @file modules/qt/src/window/core/Widgets.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt window widget ownership groups.
 */

#ifndef BLITZAR_MODULES_QT_SRC_WINDOW_CORE_WIDGETS_HPP_
#define BLITZAR_MODULES_QT_SRC_WINDOW_CORE_WIDGETS_HPP_

#include <QPointer>

class QAction;
class QCheckBox;
class QComboBox;
class QDockWidget;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QSpinBox;
class QTimer;
class QWidget;

namespace bltzr_qt {
class Graph;
class MultiView;

struct ViewWidgets final {
    explicit ViewWidgets(QWidget* parent);

    QPointer<MultiView> multiView;
    QPointer<Graph> energyGraph;
};

struct TelemetryWidgets final {
    explicit TelemetryWidgets(QWidget* parent);

    QPointer<QLabel> validationLabel;
    QPointer<QLabel> statusLabel;
    QPointer<QLabel> runtimeMetricsLabel;
    QPointer<QLabel> queueMetricsLabel;
    QPointer<QLabel> energyMetricsLabel;
    QPointer<QLabel> gpuMetricsLabel;
};

struct RunControls final {
    explicit RunControls(QWidget* parent);

    QPointer<QPushButton> pauseButton;
    QPointer<QPushButton> stepButton;
    QPointer<QPushButton> resetButton;
    QPointer<QPushButton> recoverButton;
    QPointer<QPushButton> applyConnectorButton;
    QPointer<QCheckBox> serverAutostartCheck;
    QPointer<QLineEdit> serverHostEdit;
    QPointer<QLineEdit> serverBinEdit;
    QPointer<QSpinBox> serverPortSpin;
    QPointer<QComboBox> performanceCombo;
};

struct SceneControls final {
    explicit SceneControls(QWidget* parent);

    QPointer<QPushButton> exportButton;
    QPointer<QPushButton> saveConfigButton;
    QPointer<QPushButton> loadInputButton;
    QPointer<QComboBox> simulationProfileCombo;
    QPointer<QComboBox> presetCombo;
    QPointer<QPushButton> applyPresetButton;
    QPointer<QPushButton> loadPresetButton;
};

struct PhysicsControls final {
    explicit PhysicsControls(QWidget* parent);

    QPointer<QCheckBox> sphCheck;
    QPointer<QDoubleSpinBox> sphSmoothingSpin;
    QPointer<QDoubleSpinBox> sphRestDensitySpin;
    QPointer<QDoubleSpinBox> sphGasConstantSpin;
    QPointer<QDoubleSpinBox> sphViscositySpin;
    QPointer<QDoubleSpinBox> dtSpin;
    QPointer<QComboBox> solverCombo;
    QPointer<QComboBox> integratorCombo;
    QPointer<QDoubleSpinBox> thetaSpin;
    QPointer<QDoubleSpinBox> softeningSpin;
};

struct RenderControls final {
    explicit RenderControls(QWidget* parent);

    QPointer<QSlider> zoomSlider;
    QPointer<QSlider> luminositySlider;
    QPointer<QComboBox> view3dCombo;
    QPointer<QSlider> yawSlider;
    QPointer<QSlider> pitchSlider;
    QPointer<QSlider> rollSlider;
    QPointer<QCheckBox> cullingCheck;
    QPointer<QCheckBox> lodCheck;
    QPointer<QCheckBox> octreeOverlayCheck;
    QPointer<QSpinBox> octreeOverlayDepthSpin;
    QPointer<QSpinBox> octreeOverlayOpacitySpin;
    QPointer<QCheckBox> gpuTelemetryCheck;
};

struct WorkspaceWidgets final {
    explicit WorkspaceWidgets(QWidget* parent);

    QPointer<QAction> octreeOverlayAction;
    QPointer<QAction> gpuTelemetryAction;
    QPointer<QDockWidget> controlsDock;
    QPointer<QDockWidget> energyDock;
    QPointer<QDockWidget> telemetryDock;
    QPointer<QDockWidget> validationDock;
    QPointer<QTimer> timer;
};

struct Widgets final {
    explicit Widgets(QWidget* parent);

    ViewWidgets view;
    TelemetryWidgets telemetry;
    RunControls run;
    SceneControls scene;
    PhysicsControls physics;
    RenderControls render;
    WorkspaceWidgets workspace;
};
} // namespace bltzr_qt
#endif // BLITZAR_MODULES_QT_SRC_WINDOW_CORE_WIDGETS_HPP_
