/*
 * @file modules/qt/ui/panels/RunControlPanel.cpp
 * @brief Implementation of the extracted run/connector control panel.
 */
#include "ui/panels/RunControlPanel.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace bltzr_qt {

QWidget* RunControlPanel::build(QWidget* parent, QComboBox* performanceCombo,
                                QPushButton* pauseButton, QPushButton* stepButton,
                                QPushButton* resetButton, QPushButton* recoverButton,
                                QLineEdit* serverHostEdit, QSpinBox* serverPortSpin,
                                QLineEdit* serverBinEdit, QCheckBox* serverAutostartCheck,
                                QPushButton* applyConnectorButton)
{
    QWidget* runPage = parent;
    QLayout* existing = runPage->layout();
    QVBoxLayout* runLayout = existing ? qobject_cast<QVBoxLayout*>(existing) : nullptr;
    if (!runLayout)
        runLayout = new QVBoxLayout(runPage);
    runLayout->setContentsMargins(4, 4, 4, 4);
    runLayout->setSpacing(8);

    auto* runBox = new QGroupBox("Run Control", runPage);
    auto* runBoxLayout = new QVBoxLayout(runBox);
    auto* runForm = new QFormLayout();
    runForm->addRow("performance", performanceCombo);
    auto* runActions = new QGridLayout();
    runActions->addWidget(pauseButton, 0, 0);
    runActions->addWidget(stepButton, 0, 1);
    runActions->addWidget(resetButton, 1, 0);
    runActions->addWidget(recoverButton, 1, 1);
    runBoxLayout->addLayout(runForm);
    runBoxLayout->addLayout(runActions);

    auto* connectorBox = new QGroupBox("Connector", runPage);
    auto* connectorLayout = new QVBoxLayout(connectorBox);
    auto* connectorForm = new QFormLayout();
    connectorForm->addRow("host", serverHostEdit);
    connectorForm->addRow("port", serverPortSpin);
    connectorForm->addRow("server bin", serverBinEdit);
    connectorLayout->addLayout(connectorForm);
    connectorLayout->addWidget(serverAutostartCheck);
    connectorLayout->addWidget(applyConnectorButton);
    connectorLayout->addStretch(1);

    runLayout->addWidget(runBox);
    runLayout->addWidget(connectorBox);
    runLayout->addStretch(1);
    return runPage;
}

} // namespace bltzr_qt
