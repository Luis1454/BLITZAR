#include "tests/support/qt_test_utils.hpp"

#include "tests/support/poll_utils.hpp"
#include "ui/MainWindow.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QComboBox>
#include <QDir>
#include <QEventLoop>
#include <QLabel>
#include <QLibraryInfo>
#include <QPushButton>

#include <array>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace testsupport {

QApplication *ensureQtApp()
{
    if (QApplication::instance() != nullptr) {
        return static_cast<QApplication *>(QApplication::instance());
    }
    const QString pluginRoot = QLibraryInfo::path(QLibraryInfo::PluginsPath);
    if (!pluginRoot.isEmpty()) {
        qputenv("QT_PLUGIN_PATH", pluginRoot.toUtf8());
        const QString platformsDir = QDir(pluginRoot).filePath("platforms");
        qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", platformsDir.toUtf8());
    }

    static int argc = 1;
    static std::array<char, 32768u> arg0{};
    static char *argv[] = {arg0.data(), nullptr};
    static bool initialized = false;
    if (!initialized) {
        const char *appName = "qt-mainwindow-integration-real";
        std::copy_n(appName, std::strlen(appName), arg0.data());
        arg0[std::strlen(appName)] = '\0';
        initialized = true;
    }
    static QApplication app(argc, argv);
    return &app;
}

QString findStatusLabelText(const grav_qt::MainWindow &window)
{
    const QList<QLabel *> labels = window.findChildren<QLabel *>();
    for (QLabel *label : labels) {
        if (label != nullptr && label->text().contains("state=")) {
            return label->text();
        }
    }
    return {};
}

std::string readAllFile(const std::filesystem::path &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return {};
    }
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

QComboBox *findSolverCombo(grav_qt::MainWindow &window)
{
    const QList<QComboBox *> combos = window.findChildren<QComboBox *>();
    for (QComboBox *combo : combos) {
        if (combo != nullptr
            && combo->findText("pairwise_cuda") >= 0
            && combo->findText("octree_gpu") >= 0
            && combo->findText("octree_cpu") >= 0) {
            return combo;
        }
    }
    return nullptr;
}

QPushButton *findButtonByText(grav_qt::MainWindow &window, const QString &text)
{
    const QList<QPushButton *> buttons = window.findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        if (button != nullptr && button->text() == text) {
            return button;
        }
    }
    return nullptr;
}

bool waitUntilUi(
    const std::function<bool()> &predicate,
    std::chrono::milliseconds timeout,
    std::chrono::milliseconds pollInterval)
{
    return waitUntil(predicate, timeout, pollInterval, []() {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    });
}

} // namespace testsupport
