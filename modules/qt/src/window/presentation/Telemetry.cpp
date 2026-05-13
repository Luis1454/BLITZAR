/*
 * @file modules/qt/src/window/presentation/Telemetry.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "client/common/ClientCommon.hpp"
#include "widgets/graphs/Graph.hpp"
#include "window/core/Window.hpp"
#include "widgets/viewport/MultiView.hpp"
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

QWidget* Window::buildTelemetryPane()
{
    auto* summaryPane = new QWidget(this);
    summaryPane->setObjectName("telemetrySummaryPane");
    auto* summaryLayout = new QGridLayout(summaryPane);
    summaryLayout->setContentsMargins(8, 8, 8, 8);
    summaryLayout->setHorizontalSpacing(8);
    summaryLayout->setVerticalSpacing(8);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "Session", _widgets.telemetry.statusLabel), 0, 0);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "Timing", _widgets.telemetry.runtimeMetricsLabel), 0, 1);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "Pipeline", _widgets.telemetry.queueMetricsLabel), 1, 0);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "Energy", _widgets.telemetry.energyMetricsLabel), 1, 1);
    summaryLayout->addWidget(makeSummaryCard(summaryPane, "GPU", _widgets.telemetry.gpuMetricsLabel), 2, 0, 1, 2);
    return summaryPane;
}

QWidget* Window::buildValidationPane()
{
    auto* validationPane = new QWidget(this);
    auto* validationLayout = new QVBoxLayout(validationPane);
    validationLayout->setContentsMargins(8, 8, 8, 8);
    validationLayout->setSpacing(4);
    validationLayout->addWidget(_widgets.telemetry.validationLabel);
    validationLayout->addStretch(1);
    return validationPane;
}

void Window::tick()
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
    if (!_configDirty && _widgets.physics.solverCombo && !stats.solverName.empty()) {
        const QString solverText = QString::fromStdString(stats.solverName);
        const int solverIndex = _widgets.physics.solverCombo->findText(solverText);
        if (solverIndex >= 0 && _widgets.physics.solverCombo->currentIndex() != solverIndex) {
            _widgets.physics.solverCombo->blockSignals(true);
            _widgets.physics.solverCombo->setCurrentIndex(solverIndex);
            _widgets.physics.solverCombo->blockSignals(false);
            _config.solver = stats.solverName;
        }
    }
    if (!_configDirty && _widgets.physics.integratorCombo && !stats.integratorName.empty()) {
        const QString integratorText = QString::fromStdString(stats.integratorName);
        const int integratorIndex = _widgets.physics.integratorCombo->findText(integratorText);
        if (integratorIndex >= 0 && _widgets.physics.integratorCombo->currentIndex() != integratorIndex) {
            _widgets.physics.integratorCombo->blockSignals(true);
            _widgets.physics.integratorCombo->setCurrentIndex(integratorIndex);
            _widgets.physics.integratorCombo->blockSignals(false);
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
        _widgets.view.multiView->setSnapshot(std::move(snapshot));
    }
    const std::size_t displayedParticles = _widgets.view.multiView->displayedParticleCount();
    if (_lastEnergyStep != std::numeric_limits<std::uint64_t>::max() &&
        stats.steps < _lastEnergyStep) {
        _widgets.view.energyGraph->clearHistory();
        _lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    }
    if (stats.steps != _lastEnergyStep) {
        _widgets.view.energyGraph->pushSample(stats);
        _lastEnergyStep = stats.steps;
    }
    PresentationInput presentationInput;
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
    const Presentation presentation = _presenter.present(presentationInput);
    _widgets.telemetry.statusLabel->setText(QString::fromStdString(presentation.headlineText));
    _widgets.telemetry.runtimeMetricsLabel->setText(QString::fromStdString(presentation.runtimeText));
    _widgets.telemetry.queueMetricsLabel->setText(QString::fromStdString(presentation.queueText));
    _widgets.telemetry.energyMetricsLabel->setText(QString::fromStdString(presentation.energyText));
    _widgets.telemetry.gpuMetricsLabel->setText(QString::fromStdString(presentation.gpuText));
    _widgets.run.pauseButton->blockSignals(true);
    _widgets.run.pauseButton->setChecked(stats.paused);
    _widgets.run.pauseButton->setText(stats.paused ? "Resume" : "Pause");
    _widgets.run.pauseButton->blockSignals(false);
    _widgets.run.recoverButton->setEnabled(stats.faulted);
    static auto lastConsoleTrace = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    if (now - lastConsoleTrace >= std::chrono::seconds(1)) {
        std::cout << presentation.consoleTrace << " snapshot=" << snapshotSize << "\n";
        lastConsoleTrace = now;
    }
}
} // namespace bltzr_qt
