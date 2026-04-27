/*
 * @file modules/qt/ui/panels/RunControlPanel.hpp
 * Small UI panel extracted from MainWindow to host run/connector controls.
 */
#pragma once

#include <QWidget>

namespace grav_qt {

class QComboBox;
class QPushButton;
class QLineEdit;
class QSpinBox;
class QCheckBox;

class RunControlPanel {
public:
    // Build the run control page; this function re-uses widgets owned by
    // MainWindow and arranges them into the panel returned.
    static QWidget* build(QWidget* parent, QComboBox* performanceCombo,
                          QPushButton* pauseButton, QPushButton* stepButton,
                          QPushButton* resetButton, QPushButton* recoverButton,
                          QLineEdit* serverHostEdit, QSpinBox* serverPortSpin,
                          QLineEdit* serverBinEdit, QCheckBox* serverAutostartCheck,
                          QPushButton* applyConnectorButton);
};

} // namespace grav_qt
