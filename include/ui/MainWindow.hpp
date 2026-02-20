#ifndef GRAVITY_UI_MAINWINDOW_H
#define GRAVITY_UI_MAINWINDOW_H

#include "sim/SimulationBackend.hpp"
#include "sim/SimulationConfig.hpp"

#include <QMainWindow>

#include <chrono>
#include <cstdint>
#include <string>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSlider;
class QTimer;

namespace qtui {

class EnergyGraphWidget;
class MultiViewWidget;

class MainWindow : public QMainWindow {
    public:
        MainWindow(SimulationConfig config, std::string configPath, QWidget *parent = nullptr);
        ~MainWindow() override;

    private:
        void applyConfigToBackend(bool requestReset);
        void applyConfigToUi();
        void update3DCameraFromSliders();
        void tick();

        SimulationConfig _config;
        std::string _configPath;
        SimulationBackend _backend;
        MultiViewWidget *_multiView;
        EnergyGraphWidget *_energyGraph;
        QLabel *_statusLabel;
        QPushButton *_pauseButton;
        QPushButton *_stepButton;
        QPushButton *_resetButton;
        QPushButton *_exportButton;
        QPushButton *_loadInputButton;
        QCheckBox *_sphCheck;
        QDoubleSpinBox *_sphSmoothingSpin;
        QDoubleSpinBox *_sphRestDensitySpin;
        QDoubleSpinBox *_sphGasConstantSpin;
        QDoubleSpinBox *_sphViscositySpin;
        QDoubleSpinBox *_dtSpin;
        QSlider *_zoomSlider;
        QSlider *_luminositySlider;
        QComboBox *_solverCombo;
        QComboBox *_integratorCombo;
        QComboBox *_presetCombo;
        QComboBox *_view3dCombo;
        QDoubleSpinBox *_thetaSpin;
        QDoubleSpinBox *_softeningSpin;
        QPushButton *_applyPresetButton;
        QPushButton *_loadPresetButton;
        QSlider *_yawSlider;
        QSlider *_pitchSlider;
        QSlider *_rollSlider;
        QTimer *_timer;
        std::uint64_t _lastEnergyStep;
        std::uint32_t _frontendDrawCap;
        float _uiTickFps;
        std::chrono::steady_clock::time_point _lastUiTickAt;
};

} // namespace qtui

#endif // GRAVITY_UI_MAINWINDOW_H
