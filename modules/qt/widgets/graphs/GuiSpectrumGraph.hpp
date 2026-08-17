/*
 * @file modules/qt/widgets/graphs/GuiSpectrumGraph.hpp
 * @brief Live Fourier-space density spectrum widget.
 */

#ifndef BLITZAR_MODULES_QT_SRC_WIDGETS_GRAPHS_SPECTRUMGRAPH_HPP_
#define BLITZAR_MODULES_QT_SRC_WIDGETS_GRAPHS_SPECTRUMGRAPH_HPP_

#include "types/simulation/TypSimulationTypes.hpp"
#include <QWidget>
#include <chrono>
#include <cstdint>
#include <vector>

namespace bltzr_qt {
class SpectrumGraph final : public QWidget {
public:
    explicit SpectrumGraph(QWidget* parent = nullptr);
    void clearSpectrum();
    void setSnapshot(const std::vector<RenderParticle>& snapshot,
                     float simulationTime,
                     std::uint64_t step);
    std::size_t sampledParticleCount() const;

private:
    void paintEvent(QPaintEvent* event) override;
    std::vector<float> _k;
    std::vector<float> _power;
    std::vector<std::vector<float>> _history;
    std::vector<float> _historyTimes;
    std::vector<std::uint64_t> _historySteps;
    std::size_t _sampledParticleCount;
    float _deltaRms;
    std::chrono::steady_clock::time_point _lastAnalysisAt;
    bool _hasAnalysisAt;
};
} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_SRC_WIDGETS_GRAPHS_SPECTRUMGRAPH_HPP_
