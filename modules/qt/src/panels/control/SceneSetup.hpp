/*
 * @file modules/qt/src/panels/control/SceneSetup.hpp
 * @brief Scene setup sidebar panel builder.
 */
#ifndef BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_SCENESETUP_HPP_
#define BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_SCENESETUP_HPP_

#include <QWidget>

class QComboBox;
class QPushButton;

namespace bltzr_qt {
QWidget* buildSceneSetupPanel(QWidget* parent, QComboBox* simulationProfileCombo,
                              QComboBox* presetCombo, QPushButton* applyPresetButton,
                              QPushButton* loadPresetButton, QPushButton* loadInputButton,
                              QPushButton* saveConfigButton);

} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_SCENESETUP_HPP_
