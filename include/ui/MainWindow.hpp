#ifndef GRAVITY_UI_MAINWINDOW_H
#define GRAVITY_UI_MAINWINDOW_H

#include "sim/IFrontendRuntime.hpp"
#include "sim/SimulationConfig.hpp"

#include <QMainWindow>
#include <QPointer>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QSpinBox;
class QTimer;

namespace qtui {

class EnergyGraphWidget;
class MultiViewWidget;

class MainWindow : public QMainWindow {
    public:
        MainWindow(SimulationConfig config, std::string configPath, std::unique_ptr<sim::IFrontendRuntime> runtime);
        ~MainWindow() override;

    private:
        void applyConfigToBackend(bool requestReset);
        void applyConfigToUi();
        void captureUiIntoConfig();
        void markConfigDirty(bool dirty = true);
        bool saveConfigToDisk();
        void update3DCameraFromSliders();
        void tick();

        SimulationConfig _config;
        std::string _configPath;
        std::unique_ptr<sim::IFrontendRuntime> _runtime;
        QPointer<MultiViewWidget> _multiView;
        QPointer<EnergyGraphWidget> _energyGraph;
        QPointer<QLabel> _statusLabel;
        QPointer<QPushButton> _pauseButton;
        QPointer<QPushButton> _stepButton;
        QPointer<QPushButton> _resetButton;
        QPointer<QPushButton> _recoverButton;
        QPointer<QPushButton> _reconnectButton;
        QPointer<QPushButton> _applyConnectorButton;
        QPointer<QPushButton> _exportButton;
        QPointer<QPushButton> _saveConfigButton;
        QPointer<QPushButton> _loadInputButton;
        QPointer<QCheckBox> _backendAutostartCheck;
        QPointer<QLineEdit> _backendHostEdit;
        QPointer<QLineEdit> _backendBinEdit;
        QPointer<QSpinBox> _backendPortSpin;
        QPointer<QCheckBox> _sphCheck;
        QPointer<QDoubleSpinBox> _sphSmoothingSpin;
        QPointer<QDoubleSpinBox> _sphRestDensitySpin;
        QPointer<QDoubleSpinBox> _sphGasConstantSpin;
        QPointer<QDoubleSpinBox> _sphViscositySpin;
        QPointer<QDoubleSpinBox> _dtSpin;
        QPointer<QSlider> _zoomSlider;
        QPointer<QSlider> _luminositySlider;
        QPointer<QComboBox> _solverCombo;
        QPointer<QComboBox> _integratorCombo;
        QPointer<QComboBox> _presetCombo;
        QPointer<QComboBox> _view3dCombo;
        QPointer<QDoubleSpinBox> _thetaSpin;
        QPointer<QDoubleSpinBox> _softeningSpin;
        QPointer<QPushButton> _applyPresetButton;
        QPointer<QPushButton> _loadPresetButton;
        QPointer<QSlider> _yawSlider;
        QPointer<QSlider> _pitchSlider;
        QPointer<QSlider> _rollSlider;
        QPointer<QTimer> _timer;
        std::uint64_t _lastEnergyStep;
        std::uint32_t _frontendDrawCap;
        float _uiTickFps;
        bool _configDirty;
        std::chrono::steady_clock::time_point _lastUiTickAt;
};

} // namespace qtui

#endif // GRAVITY_UI_MAINWINDOW_H
