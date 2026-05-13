/*
 * @file modules/qt/src/panels/control/Render.hpp
 * @brief Render sidebar panel builder.
 */
#ifndef BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_RENDER_HPP_
#define BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_RENDER_HPP_

#include <QWidget>

class QCheckBox;
class QComboBox;
class QPushButton;
class QSlider;
class QSpinBox;

namespace bltzr_qt {
QWidget* buildRenderPanel(QWidget* parent, QComboBox* view3dCombo, QSlider* zoomSlider,
                          QSlider* luminositySlider, QSlider* yawSlider, QSlider* pitchSlider,
                          QSlider* rollSlider, QCheckBox* cullingCheck, QCheckBox* lodCheck,
                          QCheckBox* octreeOverlayCheck, QSpinBox* octreeOverlayDepthSpin,
                          QSpinBox* octreeOverlayOpacitySpin, QCheckBox* gpuTelemetryCheck,
                          QPushButton* exportButton);

} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_RENDER_HPP_
