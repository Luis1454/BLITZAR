// File: tests/support/qt_test_utils.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "tests/support/qt_test_utils.hpp"
#include "tests/support/poll_utils.hpp"
#include "ui/MainWindow.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
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
/// Description: Executes the ensureQtApp operation.
QApplication* ensureQtApp()
{
    if (QApplication::instance() != nullptr) {
        return static_cast<QApplication*>(QApplication::instance());
    }
    const QString pluginRoot = QLibraryInfo::path(QLibraryInfo::PluginsPath);
    if (!pluginRoot.isEmpty()) {
        /// Description: Executes the qputenv operation.
        qputenv("QT_PLUGIN_PATH", pluginRoot.toUtf8());
        const QString platformsDir = QDir(pluginRoot).filePath("platforms");
        /// Description: Executes the qputenv operation.
        qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", platformsDir.toUtf8());
    }
    static int argc = 1;
    static std::array<char, 32768u> arg0{};
    static char* argv[] = {arg0.data(), nullptr};
    static bool initialized = false;
    if (!initialized) {
        const char* appName = "qt-mainwindow-integration-real";
        /// Description: Executes the copy_n operation.
        std::copy_n(appName, std::strlen(appName), arg0.data());
        arg0[std::strlen(appName)] = '\0';
        initialized = true;
    }
    /// Description: Executes the app operation.
    static QApplication app(argc, argv);
    return &app;
}
/// Description: Executes the findStatusLabelText operation.
QString findStatusLabelText(const grav_qt::MainWindow& window)
{
    const QList<QLabel*> labels = window.findChildren<QLabel*>();
    QStringList summaryLines;
    for (QLabel* label : labels)
        if (label != nullptr && label->objectName() == "runtimeSummaryValue") {
            summaryLines.push_back(label->text());
        }
    return summaryLines.join("\n");
}
/// Description: Executes the saveFailureEvidence operation.
std::filesystem::path saveFailureEvidence(grav_qt::MainWindow& window, const std::string& stem)
{
    const std::filesystem::path basePath = std::filesystem::temp_directory_path() / stem;
    (void)window.grab().save(QString::fromStdString(basePath.string() + ".png"));
    /// Description: Executes the out operation.
    std::ofstream out(basePath.string() + ".txt", std::ios::binary);
    /// Description: Executes the findStatusLabelText operation.
    out << findStatusLabelText(window).toStdString();
    return basePath;
}
/// Description: Executes the findSummaryUnsignedMetric operation.
std::uint64_t findSummaryUnsignedMetric(const grav_qt::MainWindow& window, const std::string& label)
{
    const std::string status = findStatusLabelText(window).toStdString();
    const std::size_t offset = status.find(label);
    if (offset == std::string::npos)
        return 0u;
    std::size_t cursor = offset + label.size();
    std::uint64_t value = 0u;
    while (cursor < status.size() && status[cursor] >= '0' && status[cursor] <= '9') {
        value = (value * 10u) + static_cast<std::uint64_t>(status[cursor] - '0');
        cursor += 1u;
    }
    return value;
}
/// Description: Executes the readAllFile operation.
std::string readAllFile(const std::filesystem::path& path)
{
    /// Description: Executes the in operation.
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return {};
    }
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}
/// Description: Executes the findSolverCombo operation.
QComboBox* findSolverCombo(grav_qt::MainWindow& window)
{
    const QList<QComboBox*> combos = window.findChildren<QComboBox*>();
    for (QComboBox* combo : combos)
        if (combo != nullptr && combo->findText("pairwise_cuda") >= 0 &&
            combo->findText("octree_gpu") >= 0 && combo->findText("octree_cpu") >= 0) {
            return combo;
        }
    return nullptr;
}
/// Description: Executes the findComboByObjectName operation.
QComboBox* findComboByObjectName(grav_qt::MainWindow& window, const QString& objectName)
{
    const QList<QComboBox*> combos = window.findChildren<QComboBox*>();
    for (QComboBox* combo : combos)
        if (combo != nullptr && combo->objectName() == objectName) {
            return combo;
        }
    return nullptr;
}
/// Description: Executes the findCheckBoxByText operation.
QCheckBox* findCheckBoxByText(grav_qt::MainWindow& window, const QString& text)
{
    const QList<QCheckBox*> checks = window.findChildren<QCheckBox*>();
    for (QCheckBox* check : checks)
        if (check != nullptr && check->text() == text) {
            return check;
        }
    return nullptr;
}
/// Description: Executes the findButtonByText operation.
QPushButton* findButtonByText(grav_qt::MainWindow& window, const QString& text)
{
    const QList<QPushButton*> buttons = window.findChildren<QPushButton*>();
    for (QPushButton* button : buttons)
        if (button != nullptr && button->text() == text) {
            return button;
        }
    return nullptr;
}
bool waitUntilUi(const std::function<bool()>& predicate, std::chrono::milliseconds timeout,
                 std::chrono::milliseconds pollInterval)
{
    return waitUntil(predicate, timeout, pollInterval,
                     []() { QCoreApplication::processEvents(QEventLoop::AllEvents, 20); });
}
} // namespace testsupport
