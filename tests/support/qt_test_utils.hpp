/*
 * @file tests/support/qt_test_utils.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#ifndef BLITZAR_TESTS_SUPPORT_QT_TEST_UTILS_HPP_
#define BLITZAR_TESTS_SUPPORT_QT_TEST_UTILS_HPP_
#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
/*
 * @brief Defines the qapplication type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QApplication;
/*
 * @brief Defines the qcheck box type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QCheckBox;
/*
 * @brief Defines the qcombo box type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QComboBox;
/*
 * @brief Defines the qpush button type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QPushButton;
/*
 * @brief Defines the qstring type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class QString;

namespace bltzr_qt {
class MainWindow;
} // namespace bltzr_qt

namespace testsupport {
QApplication* ensureQtApp();
QString findStatusLabelText(const bltzr_qt::MainWindow& window);
std::filesystem::path saveFailureEvidence(bltzr_qt::MainWindow& window, const std::string& stem);
std::uint64_t findSummaryUnsignedMetric(const bltzr_qt::MainWindow& window,
                                        const std::string& label);
std::string readAllFile(const std::filesystem::path& path);
QComboBox* findSolverCombo(bltzr_qt::MainWindow& window);
QComboBox* findComboByObjectName(bltzr_qt::MainWindow& window, const QString& objectName);
QCheckBox* findCheckBoxByText(bltzr_qt::MainWindow& window, const QString& text);
QPushButton* findButtonByText(bltzr_qt::MainWindow& window, const QString& text);
bool waitUntilUi(const std::function<bool()>& predicate, std::chrono::milliseconds timeout,
                 std::chrono::milliseconds pollInterval = std::chrono::milliseconds(10));
} // namespace testsupport
#endif // BLITZAR_TESTS_SUPPORT_QT_TEST_UTILS_HPP_
