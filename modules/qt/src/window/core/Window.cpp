/*
 * @file modules/qt/src/window/core/Window.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "window/core/Window.hpp"
#include "client/common/ClientCommon.hpp"
#include "support/theme/Theme.hpp"
#include "widgets/viewport/MultiView.hpp"
#include "Constants.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bltzr_qt {
Window::Window(SimulationConfig config, std::string configPath,
                       std::unique_ptr<bltzr_client::Interface> runtime)
    : QMainWindow(nullptr),
      _config(std::move(config)),
      _configPath(std::move(configPath)),
      _runtime(std::move(runtime)),
      _widgets(this),
      _workspaceLayouts(_configPath),
      _lastEnergyStep(std::numeric_limits<std::uint64_t>::max()),
      _clientDrawCap(bltzr_client::resolveClientDrawCap(_config)),
      _uiTickFps(0.0f),
      _configDirty(false),
      _lastUiTickAt()
{
    if (!_runtime) {
        throw std::invalid_argument("bltzr_qt::Window requires a valid client runtime");
    }

    setWindowTitle("N-Body Qt Client");
    menuBar()->setNativeMenuBar(false);
    setDockNestingEnabled(true);
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks);
    initializeControlState();
    setCentralWidget(_widgets.view.multiView);
    buildWorkspaceDocks(buildSidebarTabs(), buildTelemetryPane(), buildValidationPane());
    buildMenus();
    applyTheme();
    statusBar()->showMessage("Qt workspace ready", 3000);
    resize(1280, 820);
    _defaultWorkspaceGeometry = saveGeometry();
    _defaultWorkspaceState = saveState();
    const bool startupConfigValid = applyConfigToServer(false);
    markConfigDirty(false);
    applyViewSettings();
    update3DCameraFromSliders();

    _widgets.run.applyConnectorButton->setEnabled(true);
    _widgets.run.serverAutostartCheck->setEnabled(true);
    _widgets.run.serverHostEdit->setEnabled(true);
    _widgets.run.serverPortSpin->setEnabled(true);
    _widgets.run.serverBinEdit->setEnabled(true);
    connectControls();

    const std::uint32_t fps = std::max<std::uint32_t>(1u, _config.uiFpsLimit);
    if (startupConfigValid && _runtime->start())
        _widgets.workspace.timer->start(std::max(1, static_cast<int>(1000 / fps)));
    else if (!startupConfigValid)
        _widgets.telemetry.statusLabel->setText(QString("preflight validation failed; fix config before starting"));
    else
        _widgets.telemetry.statusLabel->setText(QString("server connection failed (service)"));
}

Window::~Window()
{
    _runtime->stop();
}

void Window::applyTheme()
{
    const ThemeMode mode = Theme::resolve(_config.uiTheme);
    _config.uiTheme = Theme::toConfigValue(mode);
    const QPalette palette = Theme::buildPalette(mode);
    setPalette(palette);
    QApplication::setPalette(palette);
    setAutoFillBackground(true);
}

void Window::applyViewSettings()
{
    _widgets.view.multiView->setZoom(static_cast<float>(_widgets.render.zoomSlider->value()) / kZoomSliderDivisor);
    _widgets.view.multiView->setLuminosity(_widgets.render.luminositySlider->value());
    _widgets.view.multiView->setOctreeOverlay(_widgets.render.octreeOverlayCheck->isChecked(), _widgets.render.octreeOverlayDepthSpin->value(),
                                      _widgets.render.octreeOverlayOpacitySpin->value());
    _widgets.view.multiView->setRenderSettings(_config.renderCullingEnabled, _config.renderLODEnabled,
                                  _config.renderLODNearDistance, _config.renderLODFarDistance);
    _clientDrawCap = bltzr_client::resolveClientDrawCap(_config);
    _widgets.view.multiView->setMaxDrawParticles(_clientDrawCap);
    _runtime->setRemoteSnapshotCap(_clientDrawCap);
}
} // namespace bltzr_qt
