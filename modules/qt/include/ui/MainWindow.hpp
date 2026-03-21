#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOW_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOW_HPP_

#include "client/IClientRuntime.hpp"
#include "config/SimulationConfig.hpp"
#include "ui/MainWindowController.hpp"
#include "ui/MainWindowPresenter.hpp"
#include "ui/WorkspaceLayoutStore.hpp"

#include <QByteArray>
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

namespace grav_qt {

class EnergyGraphWidget;
class MultiViewWidget;

class MainWindow : public QMainWindow {
    public:
        MainWindow(SimulationConfig config, std::string configPath, std::unique_ptr<grav_client::IClientRuntime> runtime);
        ~MainWindow() override;

    private:
        static std::string formatFromSelectedFilter(const QString &filter);
        bool applyConfigToServer(bool requestReset);
        void applyConnectorSettings(bool reconnectNow);
        void applyConfigToUi();
        void applyViewSettings();
        void captureUiIntoConfig();
        void applyPerformanceProfileToRuntime();
        void configureRemoteConnectorFromUi();
        void connectControls();
        void handleExportRequest();
        void handleLoadInputRequest();
        void handleLoadPresetRequest();
        void markConfigDirty(bool dirty = true);
        bool refreshValidationReport(bool blockOnErrors);
        void requestReconnectFromUi();
        void resetSimulationFromUi();
        void restoreDefaultWorkspace();
        void saveWorkspacePreset();
        void loadWorkspacePreset();
        void deleteWorkspacePreset();
        bool saveConfigToDisk();
        void update3DCameraFromSliders();
        void tick();

        SimulationConfig _config;
        std::string _configPath;
        std::unique_ptr<grav_client::IClientRuntime> _runtime;
        QPointer<MultiViewWidget> _multiView;
        QPointer<EnergyGraphWidget> _energyGraph;
        QPointer<QLabel> _validationLabel;
        QPointer<QLabel> _statusLabel;
        QPointer<QLabel> _runtimeMetricsLabel;
        QPointer<QLabel> _queueMetricsLabel;
        QPointer<QLabel> _energyMetricsLabel;
        QPointer<QPushButton> _pauseButton;
        QPointer<QPushButton> _stepButton;
        QPointer<QPushButton> _resetButton;
        QPointer<QPushButton> _recoverButton;
        QPointer<QPushButton> _applyConnectorButton;
        QPointer<QPushButton> _exportButton;
        QPointer<QPushButton> _saveConfigButton;
        QPointer<QPushButton> _loadInputButton;
        QPointer<QCheckBox> _serverAutostartCheck;
        QPointer<QLineEdit> _serverHostEdit;
        QPointer<QLineEdit> _serverBinEdit;
        QPointer<QSpinBox> _serverPortSpin;
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
        QPointer<QComboBox> _performanceCombo;
        QPointer<QComboBox> _simulationProfileCombo;
        QPointer<QComboBox> _presetCombo;
        QPointer<QComboBox> _view3dCombo;
        QPointer<QDoubleSpinBox> _thetaSpin;
        QPointer<QDoubleSpinBox> _softeningSpin;
        QPointer<QPushButton> _applyPresetButton;
        QPointer<QPushButton> _loadPresetButton;
        QPointer<QSlider> _yawSlider;
        QPointer<QSlider> _pitchSlider;
        QPointer<QSlider> _rollSlider;
        QPointer<QCheckBox> _cullingCheck;
        QPointer<QCheckBox> _lodCheck;
        QPointer<QTimer> _timer;
        MainWindowController _controller;
        MainWindowPresenter _presenter;
        WorkspaceLayoutStore _workspaceLayouts;
        QByteArray _defaultWorkspaceGeometry;
        QByteArray _defaultWorkspaceState;
        std::uint64_t _lastEnergyStep;
        std::uint32_t _clientDrawCap;
        float _uiTickFps;
        bool _configDirty;
        std::chrono::steady_clock::time_point _lastUiTickAt;
};

} // namespace grav_qt



#endif // GRAVITY_MODULES_QT_INCLUDE_UI_MAINWINDOW_HPP_
