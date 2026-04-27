// File: tests/support/qt_test_utils.hpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#ifndef GRAVITY_TESTS_SUPPORT_QT_TEST_UTILS_HPP_
#define GRAVITY_TESTS_SUPPORT_QT_TEST_UTILS_HPP_
#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
/// Description: Defines the QApplication data or behavior contract.
class QApplication;
/// Description: Defines the QCheckBox data or behavior contract.
class QCheckBox;
/// Description: Defines the QComboBox data or behavior contract.
class QComboBox;
/// Description: Defines the QPushButton data or behavior contract.
class QPushButton;
/// Description: Defines the QString data or behavior contract.
class QString;
namespace grav_qt {
/// Description: Defines the MainWindow data or behavior contract.
class MainWindow;
}
namespace testsupport {
/// Description: Executes the ensureQtApp operation.
QApplication* ensureQtApp();
/// Description: Executes the findStatusLabelText operation.
QString findStatusLabelText(const grav_qt::MainWindow& window);
/// Description: Executes the saveFailureEvidence operation.
std::filesystem::path saveFailureEvidence(grav_qt::MainWindow& window, const std::string& stem);
std::uint64_t findSummaryUnsignedMetric(const grav_qt::MainWindow& window,
                                        const std::string& label);
/// Description: Executes the readAllFile operation.
std::string readAllFile(const std::filesystem::path& path);
/// Description: Executes the findSolverCombo operation.
QComboBox* findSolverCombo(grav_qt::MainWindow& window);
/// Description: Executes the findComboByObjectName operation.
QComboBox* findComboByObjectName(grav_qt::MainWindow& window, const QString& objectName);
/// Description: Executes the findCheckBoxByText operation.
QCheckBox* findCheckBoxByText(grav_qt::MainWindow& window, const QString& text);
/// Description: Executes the findButtonByText operation.
QPushButton* findButtonByText(grav_qt::MainWindow& window, const QString& text);
bool waitUntilUi(const std::function<bool()>& predicate, std::chrono::milliseconds timeout,
                 std::chrono::milliseconds pollInterval = std::chrono::milliseconds(10));
} // namespace testsupport
#endif // GRAVITY_TESTS_SUPPORT_QT_TEST_UTILS_HPP_
