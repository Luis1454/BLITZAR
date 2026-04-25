#include "client/ClientRuntime.hpp"
#include "tests/support/client_utils.hpp"
#include "tests/support/qt_test_utils.hpp"
#include "ui/EnergyGraphWidget.hpp"
#include "ui/MainWindow.hpp"
#include <QCoreApplication>
#include <QEventLoop>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <gtest/gtest.h>
#include <memory>
#include <string>
namespace grav_test_qt_window_visual {
static SimulationConfig makeUiConfig()
{
    SimulationConfig config{};
    config.uiFpsLimit = 60u;
    config.exportDirectory = "exports";
    config.exportFormat = "vtk";
    config.solver = "pairwise_cuda";
    config.integrator = "euler";
    config.sphEnabled = false;
    config.dt = 0.01f;
    return config;
}
TEST(QtMainWindowTest, TST_UIX_UI_005_EnergyGraphUsesExplicitUnitsAndLegendLabels)
{
    (void)testsupport::ensureQtApp();
    const QStringList labels = grav_qt::EnergyGraphWidget::legendLabels();
    EXPECT_EQ(grav_qt::EnergyGraphWidget::energyXAxisLabel().toStdString(), "Simulation time [s]");
    EXPECT_EQ(grav_qt::EnergyGraphWidget::energyYAxisLabel().toStdString(), "Energy [J]");
    EXPECT_EQ(grav_qt::EnergyGraphWidget::driftXAxisLabel().toStdString(), "Simulation time [s]");
    EXPECT_EQ(grav_qt::EnergyGraphWidget::driftYAxisLabel().toStdString(), "Drift [%]");
    ASSERT_EQ(labels.size(), 6);
    EXPECT_EQ(labels.at(0).toStdString(), "Kinetic [J]");
    EXPECT_EQ(labels.at(1).toStdString(), "Potential [J]");
    EXPECT_EQ(labels.at(2).toStdString(), "Thermal [J]");
    EXPECT_EQ(labels.at(3).toStdString(), "Radiated [J]");
    EXPECT_EQ(labels.at(4).toStdString(), "Total [J]");
    EXPECT_EQ(labels.at(5).toStdString(), "Drift [%]");
    grav_qt::EnergyGraphWidget widget;
    widget.resize(900, 240);
    SimulationStats first{};
    first.kineticEnergy = 5.0f;
    first.potentialEnergy = -15.0f;
    first.thermalEnergy = 1.0f;
    first.radiatedEnergy = 0.5f;
    first.totalEnergy = 20.0f;
    first.energyDriftPct = -0.2f;
    first.totalTime = 0.0f;
    widget.pushSample(first);
    SimulationStats second = first;
    second.totalEnergy = 70.0f;
    second.totalTime = 1.0f;
    second.energyDriftPct = 0.35f;
    widget.pushSample(second);
    SimulationStats third = first;
    third.totalEnergy = 22.0f;
    third.totalTime = 9.0f;
    third.energyDriftPct = -0.1f;
    widget.pushSample(third);
    const auto minTotalColorYInBand = [](const QImage& image, const QColor& target, int centerX) {
        int minY = image.height();
        for (int x = std::max(0, centerX - 6); x <= std::min(image.width() - 1, centerX + 6); ++x) {
            for (int y = 40; y < image.height() - 30; ++y) {
                const QColor pixel = image.pixelColor(x, y);
                const int distance = std::abs(pixel.red() - target.red()) +
                                     std::abs(pixel.green() - target.green()) +
                                     std::abs(pixel.blue() - target.blue());
                if (distance <= 60)
                    minY = std::min(minY, y);
            }
        }
        return minY;
    };
    const auto renderWithPalette = [&](const QColor& windowColor, const QColor& textColor,
                                       const QColor& midColor) {
        QPalette palette = widget.palette();
        palette.setColor(QPalette::Window, windowColor);
        palette.setColor(QPalette::WindowText, textColor);
        palette.setColor(QPalette::Mid, midColor);
        widget.setPalette(palette);
        widget.setAutoFillBackground(true);
        QImage image(widget.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(windowColor);
        QPainter painter(&image);
        widget.render(&painter);
        painter.end();
        return image;
    };
    const QImage darkImage =
        renderWithPalette(QColor(18, 22, 28), QColor(232, 236, 242), QColor(112, 120, 132));
    const QImage lightImage =
        renderWithPalette(QColor(245, 247, 250), QColor(25, 28, 33), QColor(155, 160, 170));
    const int fullLeft = 48;
    const int fullWidth = widget.width() - 64;
    const int timeMappedX = fullLeft + static_cast<int>(fullWidth * (1.0 / 9.0));
    const int sampleMappedX = fullLeft + fullWidth / 2;
    EXPECT_LT(minTotalColorYInBand(darkImage, QColor(120, 200, 255), timeMappedX),
              minTotalColorYInBand(darkImage, QColor(120, 200, 255), sampleMappedX));
    EXPECT_LT(minTotalColorYInBand(lightImage, QColor(0, 102, 170), timeMappedX),
              minTotalColorYInBand(lightImage, QColor(0, 102, 170), sampleMappedX));
    SimulationConfig invalidConfig = makeUiConfig();
    invalidConfig.particleCount = 1u;
    invalidConfig.initConfigStyle = "detailed";
    invalidConfig.initMode = "file";
    invalidConfig.inputFile.clear();
    auto invalidRuntime = std::make_unique<grav_client::ClientRuntime>(
        "simulation.ini", testsupport::makeTransport(1u, std::string()));
    grav_qt::MainWindow invalidWindow(invalidConfig, "simulation.ini", std::move(invalidRuntime));
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    QString preflightText;
    QString statusText;
    const QList<QLabel*> labelsInWindow = invalidWindow.findChildren<QLabel*>();
    for (QLabel* label : labelsInWindow)
        if (label == nullptr)
            continue;
    if (label->text().contains("[preflight]")) {
        preflightText = label->text();
        if (label->text().contains("preflight validation failed")) {
            statusText = label->text();
        }
    }
    EXPECT_TRUE(preflightText.contains("blocked"));
    EXPECT_TRUE(preflightText.contains("input_file"));
    EXPECT_TRUE(statusText.contains("preflight validation failed"));
}
TEST(QtMainWindowTest, TST_UIX_UI_006_ResponsiveControlsAllowSubHdWindow)
{
    (void)testsupport::ensureQtApp();
    auto runtime = std::make_unique<grav_client::ClientRuntime>(
        "simulation.ini", testsupport::makeTransport(1u, std::string()));
    grav_qt::MainWindow window(makeUiConfig(), "simulation.ini", std::move(runtime));
    window.resize(1024, 768);
    window.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    EXPECT_LE(window.width(), 1100);
    EXPECT_LE(window.height(), 850);
    EXPECT_LE(window.minimumSizeHint().width(), 1100);
}
} // namespace grav_test_qt_window_visual
