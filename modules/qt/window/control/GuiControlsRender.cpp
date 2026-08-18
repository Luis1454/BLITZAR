/*
 * @file modules/qt/window/control/GuiControlsRender.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Viewport, camera, overlay, and timer control connections.
 */

#include "window/core/GuiWindow.hpp"
#include "widgets/viewport/GuiMultiView.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>

namespace bltzr_qt {
void Window::connectRenderControls()
{
    connect(_widgets.render.zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        _config.defaultZoom = zoomFromSliderValue(value);
        _widgets.view.multiView->setZoom(_config.defaultZoom);
        markConfigDirty();
    });
    connect(_widgets.render.luminositySlider, &QSlider::valueChanged, this, [this](int value) {
        _config.defaultLuminosity = value;
        _widgets.view.multiView->setLuminosity(value);
        markConfigDirty();
    });
    connect(_widgets.render.octreeOverlayDepthSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _widgets.view.multiView->setOctreeOverlay(_widgets.render.octreeOverlayCheck->isChecked(), value,
                                                         _widgets.render.octreeOverlayOpacitySpin->value());
                statusBar()->showMessage(QString("Octree overlay depth: %1").arg(value), 2000);
            });
    connect(_widgets.render.octreeOverlayOpacitySpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) {
                _widgets.view.multiView->setOctreeOverlay(_widgets.render.octreeOverlayCheck->isChecked(),
                                                         _widgets.render.octreeOverlayDepthSpin->value(), value);
                statusBar()->showMessage(QString("Octree overlay opacity: %1").arg(value), 2000);
            });
    connect(_widgets.render.view3dCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& value) {
                _widgets.view.multiView->set3DMode(
                    value == "iso" ? grav::ViewMode::Iso : grav::ViewMode::Perspective);
            });
    connect(_widgets.render.cullingCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.renderCullingEnabled = enabled;
        _widgets.view.multiView->setRenderSettings(_config.renderCullingEnabled, _config.renderLODEnabled,
                                                   _config.renderLODNearDistance, _config.renderLODFarDistance);
        markConfigDirty();
    });
    connect(_widgets.render.lodCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        _config.renderLODEnabled = enabled;
        _widgets.view.multiView->setRenderSettings(_config.renderCullingEnabled, _config.renderLODEnabled,
                                                   _config.renderLODNearDistance, _config.renderLODFarDistance);
        markConfigDirty();
    });
    connect(_widgets.render.yawSlider, &QSlider::valueChanged, this, [this]() {
        update3DCameraFromSliders();
    });
    connect(_widgets.render.pitchSlider, &QSlider::valueChanged, this, [this]() {
        update3DCameraFromSliders();
    });
    connect(_widgets.render.rollSlider, &QSlider::valueChanged, this, [this]() {
        update3DCameraFromSliders();
    });
    connect(_widgets.workspace.timer, &QTimer::timeout, this, [this]() {
        tick();
    });
}
} // namespace bltzr_qt
