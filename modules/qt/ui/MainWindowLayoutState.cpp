/*
 * @file modules/qt/ui/MainWindowLayoutState.cpp
 * @brief Thin facade delegating to layout subfolder modules.
 */

#include "ui/MainWindow.hpp"
#include "ui/mainWindow/layout/comboInitializer.hpp"
#include "ui/mainWindow/layout/numericInitializer.hpp"
#include "ui/mainWindow/layout/propertyInitializer.hpp"

namespace bltzr_qt {

void MainWindow::initializeControlState()
{
    initializeComboBoxes();
    initializeObjectNames();
    initializeSpinAndSliderValues();
    initializeLabelsAndTooltips();
}

void MainWindow::initializeComboBoxes()
{
    mainWindow::layout::ComboInitializer::initializeComboBoxes(this);
}

void MainWindow::initializeObjectNames()
{
    mainWindow::layout::PropertyInitializer::initializeObjectNames(this);
}

void MainWindow::initializeSpinAndSliderValues()
{
    mainWindow::layout::NumericInitializer::initializeSpinAndSliderValues(this);
}

void MainWindow::initializeLabelsAndTooltips()
{
    mainWindow::layout::PropertyInitializer::initializeLabelsAndTooltips(this);
}

}  // namespace bltzr_qt
