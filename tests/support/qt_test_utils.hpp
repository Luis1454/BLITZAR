#ifndef GRAVITY_TESTS_SUPPORT_QT_TEST_UTILS_HPP_
#define GRAVITY_TESTS_SUPPORT_QT_TEST_UTILS_HPP_

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>

class QApplication;
class QComboBox;
class QPushButton;
class QString;

namespace grav_qt {
class MainWindow;
}

namespace testsupport {

QApplication *ensureQtApp();

QString findStatusLabelText(const grav_qt::MainWindow &window);

std::string readAllFile(const std::filesystem::path &path);

QComboBox *findSolverCombo(grav_qt::MainWindow &window);

QPushButton *findButtonByText(grav_qt::MainWindow &window, const QString &text);

bool waitUntilUi(
    const std::function<bool()> &predicate,
    std::chrono::milliseconds timeout,
    std::chrono::milliseconds pollInterval = std::chrono::milliseconds(10));

} // namespace testsupport


#endif // GRAVITY_TESTS_SUPPORT_QT_TEST_UTILS_HPP_
