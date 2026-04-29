/*
 * @file modules/qt/ui/panels/RenderControlPanel.cpp
 * @brief Implementation of the render sidebar panel.
 */

#include "ui/panels/RenderControlPanel.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

namespace bltzr_qt {

QWidget* RenderControlPanel::build(QWidget* parent, QComboBox* view3dCombo, QSlider* zoomSlider,
                                   QSlider* luminositySlider, QSlider* yawSlider,
                                   QSlider* pitchSlider, QSlider* rollSlider,
                                   QCheckBox* cullingCheck, QCheckBox* lodCheck,
                                   QCheckBox* octreeOverlayCheck, QSpinBox* octreeOverlayDepthSpin,
                                   QSpinBox* octreeOverlayOpacitySpin, QCheckBox* gpuTelemetryCheck,
                                   QPushButton* exportButton)
{
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(8);

    auto* cameraBox = new QGroupBox("View & Camera", page);
    auto* cameraLayout = new QGridLayout(cameraBox);
    cameraLayout->addWidget(new QLabel("zoom", page), 0, 0);
    cameraLayout->addWidget(zoomSlider, 0, 1);
    cameraLayout->addWidget(new QLabel("luminosity", page), 0, 2);
    cameraLayout->addWidget(luminositySlider, 0, 3);
    cameraLayout->addWidget(new QLabel("3D view", page), 1, 0);
    cameraLayout->addWidget(view3dCombo, 1, 1);
    cameraLayout->addWidget(new QLabel("yaw", page), 1, 2);
    cameraLayout->addWidget(yawSlider, 1, 3);
    cameraLayout->addWidget(new QLabel("pitch", page), 2, 0);
    cameraLayout->addWidget(pitchSlider, 2, 1);
    cameraLayout->addWidget(new QLabel("roll", page), 2, 2);
    cameraLayout->addWidget(rollSlider, 2, 3);
    cameraLayout->addWidget(cullingCheck, 3, 0);
    cameraLayout->addWidget(lodCheck, 3, 1);

    auto* overlayBox = new QGroupBox("Octree Overlay", page);
    auto* overlayLayout = new QFormLayout(overlayBox);
    overlayLayout->addRow(octreeOverlayCheck);
    overlayLayout->addRow("depth", octreeOverlayDepthSpin);
    overlayLayout->addRow("opacity", octreeOverlayOpacitySpin);

    auto* exportBox = new QGroupBox("Export", page);
    auto* exportLayout = new QVBoxLayout(exportBox);
    exportLayout->addWidget(exportButton);
    exportLayout->addWidget(gpuTelemetryCheck);
    exportLayout->addStretch(1);

    layout->addWidget(cameraBox);
    layout->addWidget(overlayBox);
    layout->addWidget(exportBox);
    layout->addStretch(1);
    return page;
}

} // namespace bltzr_qt
