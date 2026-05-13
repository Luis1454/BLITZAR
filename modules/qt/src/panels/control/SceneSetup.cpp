/*
 * @file modules/qt/src/panels/control/SceneSetup.cpp
 * @brief Implementation of the scene setup sidebar panel.
 */

#include "panels/control/SceneSetup.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace bltzr_qt {

QWidget* buildSceneSetupPanel(QWidget* parent, QComboBox* simulationProfileCombo,
                              QComboBox* presetCombo, QPushButton* applyPresetButton,
                              QPushButton* loadPresetButton, QPushButton* loadInputButton,
                              QPushButton* saveConfigButton)
{
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(8);

    auto* setupBox = new QGroupBox("Scene Setup", page);
    auto* setupLayout = new QVBoxLayout(setupBox);
    auto* setupForm = new QFormLayout();
    setupForm->addRow("profile", simulationProfileCombo);
    setupForm->addRow("preset", presetCombo);
    setupLayout->addLayout(setupForm);
    setupLayout->addWidget(applyPresetButton);
    setupLayout->addWidget(loadPresetButton);
    setupLayout->addWidget(loadInputButton);

    auto* projectBox = new QGroupBox("Project", page);
    auto* projectLayout = new QVBoxLayout(projectBox);
    projectLayout->addWidget(saveConfigButton);
    projectLayout->addStretch(1);

    layout->addWidget(setupBox);
    layout->addWidget(projectBox);
    layout->addStretch(1);
    return page;
}

} // namespace bltzr_qt
