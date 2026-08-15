/*
 * @file modules/qt/src/panels/control/Render.cpp
 * @brief Implementation of the render sidebar panel.
 */

#include "panels/control/Render.hpp"
#include "panels/control/Disclosure.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

namespace bltzr_qt {

QWidget* buildRenderPanel(QWidget* parent, QComboBox* view3dCombo, QSlider* zoomSlider,
                          QSlider* luminositySlider, QSlider* yawSlider,
                          QSlider* pitchSlider, QSlider* rollSlider,
                          QCheckBox* cullingCheck, QCheckBox* lodCheck,
                          QCheckBox* octreeOverlayCheck, QSpinBox* octreeOverlayDepthSpin,
                          QSpinBox* octreeOverlayOpacitySpin, QCheckBox* gpuTelemetryCheck,
                          QPushButton* exportButton, QProgressBar* exportProgress)
{
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(8);

    auto* cameraBox = new QGroupBox("Viewport", page);
    auto* cameraLayout = new QGridLayout(cameraBox);
    cameraLayout->addWidget(new QLabel("Zoom", page), 0, 0);
    cameraLayout->addWidget(zoomSlider, 0, 1);
    cameraLayout->addWidget(new QLabel("Brightness", page), 0, 2);
    cameraLayout->addWidget(luminositySlider, 0, 3);
    cameraLayout->addWidget(new QLabel("View", page), 1, 0);
    cameraLayout->addWidget(view3dCombo, 1, 1);
    cameraLayout->addWidget(cullingCheck, 1, 2);
    cameraLayout->addWidget(lodCheck, 1, 3);

    auto* orientationPage = new QWidget(page);
    auto* orientationLayout = new QFormLayout(orientationPage);
    orientationLayout->addRow("Yaw", yawSlider);
    orientationLayout->addRow("Pitch", pitchSlider);
    orientationLayout->addRow("Roll", rollSlider);

    auto* overlayBox = new QGroupBox("Octree overlay", page);
    auto* overlayLayout = new QFormLayout(overlayBox);
    overlayLayout->addRow(octreeOverlayCheck);
    overlayLayout->addRow("depth", octreeOverlayDepthSpin);
    overlayLayout->addRow("opacity", octreeOverlayOpacitySpin);

    auto* exportBox = new QGroupBox("Export", page);
    auto* exportLayout = new QVBoxLayout(exportBox);
    exportLayout->addWidget(exportButton);
    exportLayout->addWidget(exportProgress);
    layout->addWidget(cameraBox);
    layout->addWidget(exportBox);
    layout->addWidget(buildDisclosure(page, "Camera orientation", orientationPage));
    layout->addWidget(buildDisclosure(page, "Diagnostics overlay", overlayBox));
    layout->addWidget(buildDisclosure(page, "GPU telemetry", gpuTelemetryCheck));
    layout->addStretch(1);
    return page;
}

} // namespace bltzr_qt
