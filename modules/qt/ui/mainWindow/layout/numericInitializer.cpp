/*
 * @file modules/qt/ui/mainWindow/layout/numericInitializer.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Numeric control initialization implementations.
 */

#include "ui/MainWindow.hpp"
#include "ui/mainWindow/layout/numericInitializer.hpp"
#include <QDoubleSpinBox>
#include <QSlider>
#include <QSpinBox>

namespace bltzr_qt::mainWindow::layout {

void NumericInitializer::initializeSpinAndSliderValues(MainWindow* mainWindow)
{
    if (mainWindow == nullptr) {
        return;
    }

    mainWindow->_serverPortSpin->setRange(1, 65535);
    mainWindow->_serverPortSpin->setValue(4242);

    mainWindow->_dtSpin->setRange(0.00001, 1.0);
    mainWindow->_dtSpin->setDecimals(5);
    mainWindow->_dtSpin->setSingleStep(0.001);
    mainWindow->_dtSpin->setValue(mainWindow->_config.dt);

    mainWindow->_thetaSpin->setRange(0.05, 4.0);
    mainWindow->_thetaSpin->setDecimals(3);
    mainWindow->_thetaSpin->setSingleStep(0.05);
    mainWindow->_thetaSpin->setValue(mainWindow->_config.octreeTheta);

    mainWindow->_softeningSpin->setRange(0.0001, 5.0);
    mainWindow->_softeningSpin->setDecimals(4);
    mainWindow->_softeningSpin->setSingleStep(0.05);
    mainWindow->_softeningSpin->setValue(mainWindow->_config.octreeSoftening);

    mainWindow->_sphSmoothingSpin->setRange(0.05, 10.0);
    mainWindow->_sphSmoothingSpin->setDecimals(3);
    mainWindow->_sphSmoothingSpin->setSingleStep(0.05);
    mainWindow->_sphSmoothingSpin->setValue(mainWindow->_config.sphSmoothingLength);

    mainWindow->_sphRestDensitySpin->setRange(0.05, 100.0);
    mainWindow->_sphRestDensitySpin->setDecimals(3);
    mainWindow->_sphRestDensitySpin->setSingleStep(0.1);
    mainWindow->_sphRestDensitySpin->setValue(mainWindow->_config.sphRestDensity);

    mainWindow->_sphGasConstantSpin->setRange(0.01, 100.0);
    mainWindow->_sphGasConstantSpin->setDecimals(3);
    mainWindow->_sphGasConstantSpin->setSingleStep(0.1);
    mainWindow->_sphGasConstantSpin->setValue(mainWindow->_config.sphGasConstant);

    mainWindow->_sphViscositySpin->setRange(0.0, 10.0);
    mainWindow->_sphViscositySpin->setDecimals(3);
    mainWindow->_sphViscositySpin->setSingleStep(0.01);
    mainWindow->_sphViscositySpin->setValue(mainWindow->_config.sphViscosity);

    mainWindow->_zoomSlider->setRange(1, 400);
    mainWindow->_zoomSlider->setSingleStep(1);
    mainWindow->_zoomSlider->setValue(static_cast<int>(mainWindow->_config.defaultZoom * 10.0f));

    mainWindow->_luminositySlider->setRange(0, 255);
    mainWindow->_luminositySlider->setSingleStep(1);
    mainWindow->_luminositySlider->setValue(mainWindow->_config.defaultLuminosity);

    mainWindow->_yawSlider->setRange(-180, 180);
    mainWindow->_pitchSlider->setRange(-180, 180);
    mainWindow->_rollSlider->setRange(-180, 180);
    mainWindow->_yawSlider->setSingleStep(1);
    mainWindow->_pitchSlider->setSingleStep(1);
    mainWindow->_rollSlider->setSingleStep(1);
}

} // namespace bltzr_qt::mainWindow::layout
