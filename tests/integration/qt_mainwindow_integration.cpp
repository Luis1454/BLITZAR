#include "sim/FrontendRuntime.hpp"
#include "sim/ILocalBackend.hpp"
#include "tests/integration/RealBackendHarness.hpp"
#include "ui/MainWindow.hpp"

#include <gtest/gtest.h>

#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QLabel>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

namespace sim {

std::unique_ptr<ILocalBackend> createLocalBackend(const std::string &)
{
    return nullptr;
}

} // namespace sim

namespace {

QApplication *ensureApp()
{
    if (QApplication::instance() != nullptr) {
        return static_cast<QApplication *>(QApplication::instance());
    }
    static int argc = 1;
    static char arg0[] = "qt-mainwindow-integration-real";
    static char *argv[] = {arg0, nullptr};
    static QApplication app(argc, argv);
    return &app;
}

QString findStatusLabelText(const qtui::MainWindow &window)
{
    const QList<QLabel *> labels = window.findChildren<QLabel *>();
    for (QLabel *label : labels) {
        if (label != nullptr && label->text().contains("state=")) {
            return label->text();
        }
    }
    return {};
}

bool waitUntil(const std::function<bool()> &predicate, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return predicate();
}

sim::FrontendTransportArgs makeTransport(std::uint16_t port, const std::string &backendExecutable)
{
    sim::FrontendTransportArgs transport{};
    transport.remoteMode = true;
    transport.remoteHost = "127.0.0.1";
    transport.remotePort = port;
    transport.remoteAutoStart = false;
    transport.backendExecutable = backendExecutable;
    transport.remoteCommandTimeoutMs = 80u;
    transport.remoteStatusTimeoutMs = 40u;
    transport.remoteSnapshotTimeoutMs = 120u;
    return transport;
}

SimulationConfig makeUiConfig()
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

TEST(QtMainWindowIntegration, ConstructsAndTicksWithRealRuntime)
{
    (void)ensureApp();

    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;

    auto runtime = std::make_unique<sim::FrontendRuntime>(
        "simulation.ini",
        makeTransport(backend.port(), backend.executablePath()));
    qtui::MainWindow window(makeUiConfig(), "simulation.ini", std::move(runtime));

    ASSERT_TRUE(waitUntil([&]() {
        const QString status = findStatusLabelText(window);
        return status.contains("link=connected") && status.contains("owner=external");
    }, std::chrono::milliseconds(5000)));

    backend.stop();
}

TEST(QtMainWindowIntegration, ShowsReconnectingWhenBackendStopsAndRecovers)
{
    (void)ensureApp();

    RealBackendHarness backend;
    std::string startError;
    ASSERT_TRUE(backend.start(startError)) << startError;
    const std::uint16_t fixedPort = backend.port();

    auto runtime = std::make_unique<sim::FrontendRuntime>(
        "simulation.ini",
        makeTransport(fixedPort, backend.executablePath()));
    qtui::MainWindow window(makeUiConfig(), "simulation.ini", std::move(runtime));

    ASSERT_TRUE(waitUntil([&]() {
        return findStatusLabelText(window).contains("link=connected");
    }, std::chrono::milliseconds(5000)));

    backend.stop();
    ASSERT_TRUE(waitUntil([&]() {
        return findStatusLabelText(window).contains("link=reconnecting");
    }, std::chrono::milliseconds(5000)));

    ASSERT_TRUE(backend.start(startError, fixedPort)) << startError;
    ASSERT_TRUE(waitUntil([&]() {
        return findStatusLabelText(window).contains("link=connected");
    }, std::chrono::milliseconds(7000)));

    backend.stop();
}

} // namespace
