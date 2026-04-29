/*
 * @file modules/qt/include/ui/panels/SceneSetupPanel.hpp
 * @brief Scene setup sidebar panel builder.
 */
#ifndef BLITZAR_MODULES_QT_INCLUDE_UI_PANELS_SCENESETUPPANEL_HPP_
#define BLITZAR_MODULES_QT_INCLUDE_UI_PANELS_SCENESETUPPANEL_HPP_

#include <QWidget>

class QComboBox;
class QPushButton;

namespace bltzr_qt {

class SceneSetupPanel {
public:
    static QWidget* build(QWidget* parent, QComboBox* simulationProfileCombo,
                          QComboBox* presetCombo, QPushButton* applyPresetButton,
                          QPushButton* loadPresetButton, QPushButton* loadInputButton,
                          QPushButton* saveConfigButton);
};

} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_INCLUDE_UI_PANELS_SCENESETUPPANEL_HPP_
