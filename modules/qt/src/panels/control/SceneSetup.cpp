/*
 * @file modules/qt/src/panels/control/SceneSetup.cpp
 * @brief Implementation of the scene setup sidebar panel.
 */

#include "panels/control/SceneSetup.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
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
    layout->setSpacing(4);

    auto* setupBox = new QGroupBox("Case", page);
    auto* setupLayout = new QHBoxLayout(setupBox);
    setupLayout->setContentsMargins(6, 5, 6, 5);
    setupLayout->addWidget(new QLabel("Profile", setupBox));
    setupLayout->addWidget(simulationProfileCombo, 1);
    setupLayout->addWidget(new QLabel("Preset", setupBox));
    setupLayout->addWidget(presetCombo, 1);
    setupLayout->addWidget(applyPresetButton);
    setupLayout->addWidget(loadPresetButton);
    setupLayout->addWidget(loadInputButton);
    setupLayout->addWidget(saveConfigButton);

    layout->addWidget(setupBox);
    return page;
}

} // namespace bltzr_qt
