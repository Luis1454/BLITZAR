/*
 * @file modules/qt/ui/mainWindow/layout/comboInitializer.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Combo box initialization implementations.
 */

#include "ui/MainWindow.hpp"
#include "ui/UiEnums.hpp"
#include "ui/mainWindow/layout/comboInitializer.hpp"
#include <QComboBox>

namespace bltzr_qt::mainWindow::layout {

void ComboInitializer::initializeComboBoxes(MainWindow* mainWindow)
{
    if (mainWindow == nullptr) {
        return;
    }

    mainWindow->_solverCombo->blockSignals(true);
    mainWindow->_integratorCombo->blockSignals(true);
    mainWindow->_performanceCombo->blockSignals(true);
    mainWindow->_simulationProfileCombo->blockSignals(true);
    mainWindow->_presetCombo->blockSignals(true);
    mainWindow->_view3dCombo->blockSignals(true);

    mainWindow->_solverCombo->clear();
    mainWindow->_solverCombo->addItem("pairwise_cuda");
    mainWindow->_solverCombo->addItem("octree_gpu");
    mainWindow->_solverCombo->addItem("octree_cpu");

    mainWindow->_integratorCombo->clear();
    mainWindow->_integratorCombo->addItem("euler");
    mainWindow->_integratorCombo->addItem("rk4");

    mainWindow->_performanceCombo->clear();
    mainWindow->_performanceCombo->addItem("interactive");
    mainWindow->_performanceCombo->addItem("balanced");
    mainWindow->_performanceCombo->addItem("quality");
    mainWindow->_performanceCombo->addItem("custom");

    mainWindow->_simulationProfileCombo->clear();
    mainWindow->_simulationProfileCombo->addItem("disk_orbit");
    mainWindow->_simulationProfileCombo->addItem("galaxy_collision");
    mainWindow->_simulationProfileCombo->addItem("solar_system");
    mainWindow->_simulationProfileCombo->addItem("binary_star");
    mainWindow->_simulationProfileCombo->addItem("random_cloud");
    mainWindow->_simulationProfileCombo->addItem("cube_random");
    mainWindow->_simulationProfileCombo->addItem("sphere_random");

    mainWindow->_presetCombo->clear();
    mainWindow->_presetCombo->addItem("disk_orbit");
    mainWindow->_presetCombo->addItem("galaxy_collision");
    mainWindow->_presetCombo->addItem("solar_system");
    mainWindow->_presetCombo->addItem("binary_star");
    mainWindow->_presetCombo->addItem("random_cloud");
    mainWindow->_presetCombo->addItem("cube_random");
    mainWindow->_presetCombo->addItem("sphere_random");
    mainWindow->_presetCombo->addItem("file");

    mainWindow->_view3dCombo->clear();
    mainWindow->_view3dCombo->addItem("iso");
    mainWindow->_view3dCombo->addItem("perspective");

    const int solverIndex = mainWindow->_solverCombo->findText(QString::fromStdString(mainWindow->_config.solver));
    const int integratorIndex =
        mainWindow->_integratorCombo->findText(QString::fromStdString(mainWindow->_config.integrator));
    const int performanceIndex = mainWindow->_performanceCombo->findText(
        QString::fromStdString(mainWindow->_config.performanceProfile));
    const int simulationProfileIndex = mainWindow->_simulationProfileCombo->findText(
        QString::fromStdString(mainWindow->_config.simulationProfile));
    const int presetIndex = mainWindow->_presetCombo->findText(
        QString::fromStdString(mainWindow->_config.presetStructure));

    mainWindow->_solverCombo->setCurrentIndex(solverIndex >= 0 ? solverIndex : 0);
    mainWindow->_integratorCombo->setCurrentIndex(integratorIndex >= 0 ? integratorIndex : 0);
    mainWindow->_performanceCombo->setCurrentIndex(performanceIndex >= 0 ? performanceIndex : 0);
    mainWindow->_simulationProfileCombo->setCurrentIndex(
        simulationProfileIndex >= 0 ? simulationProfileIndex : 0);
    mainWindow->_presetCombo->setCurrentIndex(presetIndex >= 0 ? presetIndex : 0);
    mainWindow->_view3dCombo->setCurrentIndex(1);

    mainWindow->_solverCombo->blockSignals(false);
    mainWindow->_integratorCombo->blockSignals(false);
    mainWindow->_performanceCombo->blockSignals(false);
    mainWindow->_simulationProfileCombo->blockSignals(false);
    mainWindow->_presetCombo->blockSignals(false);
    mainWindow->_view3dCombo->blockSignals(false);
}

} // namespace bltzr_qt::mainWindow::layout
