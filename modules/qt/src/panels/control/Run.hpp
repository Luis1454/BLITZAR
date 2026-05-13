/*
 * @file modules/qt/src/panels/control/Run.hpp
 * @brief Small UI panel extracted from Window to host run/connector controls.
 */
#ifndef BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_RUN_HPP_
#define BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_RUN_HPP_

#include <QWidget>

class QComboBox;
class QPushButton;
class QLineEdit;
class QSpinBox;
class QCheckBox;

namespace bltzr_qt {
QWidget* buildRunPanel(QWidget* parent, QComboBox* performanceCombo, QPushButton* pauseButton,
                       QPushButton* stepButton, QPushButton* resetButton,
                       QPushButton* recoverButton, QLineEdit* serverHostEdit,
                       QSpinBox* serverPortSpin, QLineEdit* serverBinEdit,
                       QCheckBox* serverAutostartCheck, QPushButton* applyConnectorButton);

} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_SRC_PANELS_CONTROL_RUN_HPP_
