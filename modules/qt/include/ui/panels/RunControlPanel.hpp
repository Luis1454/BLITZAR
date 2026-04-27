/*
 * @file modules/qt/include/ui/panels/RunControlPanel.hpp
 * @brief Small UI panel extracted from MainWindow to host run/connector controls.
 */
#ifndef GRAVITY_MODULES_QT_INCLUDE_UI_PANELS_RUNCONTROLPANEL_HPP_
#define GRAVITY_MODULES_QT_INCLUDE_UI_PANELS_RUNCONTROLPANEL_HPP_

#include <QWidget>

class QComboBox;
class QPushButton;
class QLineEdit;
class QSpinBox;
class QCheckBox;

namespace grav_qt {

class RunControlPanel {
public:
    static QWidget* build(QWidget* parent, QComboBox* performanceCombo,
                          QPushButton* pauseButton, QPushButton* stepButton,
                          QPushButton* resetButton, QPushButton* recoverButton,
                          QLineEdit* serverHostEdit, QSpinBox* serverPortSpin,
                          QLineEdit* serverBinEdit, QCheckBox* serverAutostartCheck,
                          QPushButton* applyConnectorButton);
};

} // namespace grav_qt

#endif // GRAVITY_MODULES_QT_INCLUDE_UI_PANELS_RUNCONTROLPANEL_HPP_
