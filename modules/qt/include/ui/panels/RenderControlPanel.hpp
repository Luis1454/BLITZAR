/*
 * @file modules/qt/include/ui/panels/RenderControlPanel.hpp
 * @brief Render sidebar panel builder.
 */
#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_PANELS_RENDERCONTROLPANEL_HPP_
#define BLITZAR_MODULES_QT_INCLUDE_UI_PANELS_RENDERCONTROLPANEL_HPP_

#include <QWidget>

class QCheckBox;
class QComboBox;
class QPushButton;
class QSlider;
class QSpinBox;

namespace bltzr_qt {

class RenderControlPanel {
public:
    static QWidget* build(QWidget* parent, QComboBox* view3dCombo, QSlider* zoomSlider,
                          QSlider* luminositySlider, QSlider* yawSlider, QSlider* pitchSlider,
                          QSlider* rollSlider, QCheckBox* cullingCheck, QCheckBox* lodCheck,
                          QCheckBox* octreeOverlayCheck, QSpinBox* octreeOverlayDepthSpin,
                          QSpinBox* octreeOverlayOpacitySpin, QCheckBox* gpuTelemetryCheck,
                          QPushButton* exportButton);
};

} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_INCLUDE_UI_PANELS_RENDERCONTROLPANEL_HPP_
