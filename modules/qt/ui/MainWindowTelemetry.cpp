/*
 * @file modules/qt/ui/MainWindowTelemetry.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "client/ClientCommon.hpp"
#include "ui/EnergyGraphWidget.hpp"
#include "ui/MainWindow.hpp"
#include "ui/MultiViewWidget.hpp"
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace bltzr_qt {
static QFrame* makeSummaryCard(QWidget* parent, const QString& title, QLabel* content)
{
    auto* card = new QFrame(parent);
    card->setObjectName("runtimeCard");
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(4);
    auto* titleLabel = new QLabel(title, card);
    titleLabel->setObjectName("runtimeCardTitle");
    layout->addWidget(titleLabel);
    layout->addWidget(content);
    layout->addStretch(1);
    return card;
}

QWidget* MainWindow::buildTelemetryPane()
{
    auto* summaryPane = new QWidget(this);
    summaryPane->setObjectName("telemetrySummaryPane");
    auto* summaryLayout = new QGridLayout(summaryPane);
    summaryLayout->setContentsMargins(8, 8, 8, 8);
    summaryLayout->setHorizontalSpacing(8);
    summaryLayout->setVerticalSpacing(8);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "Session", _statusLabel), 0, 0);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "Timing", _runtimeMetricsLabel), 0, 1);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "Pipeline", _queueMetricsLabel), 1, 0);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "Energy", _energyMetricsLabel), 1, 1);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "GPU", _gpuMetricsLabel), 2, 0, 1, 2);
    return summaryPane;
}

QWidget* MainWindow::buildValidationPane()
{
    auto* validationPane = new QWidget(this);
    auto* validationLayout = new QVBoxLayout(validationPane);
    validationLayout->setContentsMargins(8, 8, 8, 8);
    validationLayout->setSpacing(4);
    validationLayout->addWidget(_validationLabel);
    validationLayout->addStretch(1);
    return validationPane;
}

void MainWindow::tick()
{
    const auto uiNow = std::chrono::steady_clock::now();
    if (_lastUiTickAt.time_since_epoch().count() != 0) {
        const std::chrono::duration<float> uiDt = uiNow - _lastUiTickAt;
        if (uiDt.count() > 1e-6f) {
            const float inst = 1.0f / uiDt.count();
            _uiTickFps = (_uiTickFps <= 0.0f) ? inst : (0.9f * _uiTickFps + 0.1f * inst);
        }
    }
    _lastUiTickAt = uiNow;
    std::vector<RenderParticle> snapshot;
    const SimulationStats stats = _runtime->getCachedStats();
    if (!_configDirty && _solverCombo && !stats.solverName.empty()) {
        const QString solverText = QString::fromStdString(stats.solverName);
        const int solverIndex = _solverCombo->findText(solverText);
        if (solverIndex >= 0 && _solverCombo->currentIndex() != solverIndex) {
            _solverCombo->blockSignals(true);
            _solverCombo->setCurrentIndex(solverIndex);
            _solverCombo->blockSignals(false);
            _config.solver = stats.solverName;
        }
    }
    if (!_configDirty && _integratorCombo && !stats.integratorName.empty()) {
        const QString integratorText = QString::fromStdString(stats.integratorName);
        const int integratorIndex = _integratorCombo->findText(integratorText);
        if (integratorIndex >= 0 && _integratorCombo->currentIndex() != integratorIndex) {
            _integratorCombo->blockSignals(true);
            _integratorCombo->setCurrentIndex(integratorIndex);
            _integratorCombo->blockSignals(false);
            _config.integrator = stats.integratorName;
        }
    }
    std::size_t snapshotSize = 0u;
    std::uint32_t consumedSnapshotLatencyMs = std::numeric_limits<std::uint32_t>::max();
    std::optional<bltzr_client::ConsumedSnapshot> consumedSnapshot =
        _runtime->consumeLatestSnapshot();
    const bool gotSnapshot = consumedSnapshot.has_value();
    if (gotSnapshot) {
        snapshotSize = consumedSnapshot->sourceSize;
        consumedSnapshotLatencyMs = consumedSnapshot->latencyMs;
        snapshot = std::move(consumedSnapshot->particles);
    }
    const bltzr_client::SnapshotPipelineState snapshotPipeline = _runtime->snapshotPipelineState();
    const std::string linkLabel = _runtime->linkStateLabel();
    const std::string ownerLabel = _runtime->serverOwnerLabel();
    const std::uint32_t statsAgeMs = _runtime->statsAgeMs();
    const std::uint32_t snapshotAgeMs = _runtime->snapshotAgeMs();
    if (gotSnapshot) {
        _multiView->setSnapshot(std::move(snapshot));
    }
    const std::size_t displayedParticles = _multiView->displayedParticleCount();
    if (_lastEnergyStep != std::numeric_limits<std::uint64_t>::max() &&
        stats.steps < _lastEnergyStep) {
        _energyGraph->clearHistory();
        _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    }
    if (stats.steps != _lastEnergyStep) {
        _energyGraph->pushSample(stats);
        _lastEnergyStep = stats.steps;
    }
    MainWindowPresentationInput presentationInput;
    presentationInput.stats = stats;
    presentationInput.snapshotPipeline = snapshotPipeline;
    presentationInput.linkLabel = linkLabel;
    presentationInput.ownerLabel = ownerLabel;
    presentationInput.performanceProfile =
        stats.performanceProfile.empty() ? _config.performanceProfile : stats.performanceProfile;
    presentationInput.displayedParticles = displayedParticles;
    presentationInput.clientDrawCap = _clientDrawCap;
    presentationInput.statsAgeMs = statsAgeMs;
    presentationInput.snapshotAgeMs = snapshotAgeMs;
    presentationInput.snapshotLatencyMs = consumedSnapshotLatencyMs;
    presentationInput.uiTickFps = _uiTickFps;
    const MainWindowPresentation presentation = _presenter.present(presentationInput);
    _statusLabel->setText(QString::fromStdString(presentation.headlineText));
    _runtimeMetricsLabel->setText(QString::fromStdString(presentation.runtimeText));
    _queueMetricsLabel->setText(QString::fromStdString(presentation.queueText));
    _energyMetricsLabel->setText(QString::fromStdString(presentation.energyText));
    _gpuMetricsLabel->setText(QString::fromStdString(presentation.gpuText));
    _pauseButton->blockSignals(true);
    _pauseButton->setChecked(stats.paused);
    _pauseButton->setText(stats.paused ? "Resume" : "Pause");
    _pauseButton->blockSignals(false);
    _recoverButton->setEnabled(stats.faulted);
    static auto lastConsoleTrace = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    if (now - lastConsoleTrace >= std::chrono::seconds(1)) {
        std::cout << presentation.consoleTrace << " snapshot=" << snapshotSize << "\n";
        lastConsoleTrace = now;
    }
}
} // namespace bltzr_qt
