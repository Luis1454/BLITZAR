/*
 * @file modules/qt/src/widgets/graphs/SpectrumGraph.cpp
 * @brief Live Fourier-space density spectrum widget.
 */

#include "widgets/graphs/SpectrumGraph.hpp"
#include "Constants.hpp"

#include <QPainter>
#include <QPalette>
#include <QSizePolicy>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <limits>
#include <vector>

namespace bltzr_qt {
namespace {
constexpr std::size_t kGridSize = 64u;
constexpr std::size_t kSpectrumBins = 32u;
constexpr std::size_t kHistoryColumns = 240u;
constexpr std::chrono::milliseconds kAnalysisInterval(100);
using Complex = std::complex<float>;

std::size_t gridIndex(std::size_t x, std::size_t y, std::size_t z)
{
    return (x * kGridSize + y) * kGridSize + z;
}

void fftLine(std::vector<Complex>& line)
{
    const std::size_t size = line.size();
    for (std::size_t i = 1u, j = 0u; i < size; ++i) {
        std::size_t bit = size >> 1u;
        for (; (j & bit) != 0u; bit >>= 1u)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(line[i], line[j]);
    }
    for (std::size_t length = 2u; length <= size; length <<= 1u) {
        const float angle = -2.0f * kPi /
                            static_cast<float>(length);
        const Complex root(std::cos(angle), std::sin(angle));
        for (std::size_t start = 0u; start < size; start += length) {
            Complex factor(1.0f, 0.0f);
            const std::size_t half = length >> 1u;
            for (std::size_t offset = 0u; offset < half; ++offset) {
                const Complex even = line[start + offset];
                const Complex odd = factor * line[start + offset + half];
                line[start + offset] = even + odd;
                line[start + offset + half] = even - odd;
                factor *= root;
            }
        }
    }
}

void transformGrid(std::vector<Complex>& grid)
{
    std::vector<Complex> line(kGridSize);
    for (std::size_t x = 0u; x < kGridSize; ++x) {
        for (std::size_t y = 0u; y < kGridSize; ++y) {
            for (std::size_t z = 0u; z < kGridSize; ++z)
                line[z] = grid[gridIndex(x, y, z)];
            fftLine(line);
            for (std::size_t z = 0u; z < kGridSize; ++z)
                grid[gridIndex(x, y, z)] = line[z];
        }
    }
    for (std::size_t x = 0u; x < kGridSize; ++x) {
        for (std::size_t z = 0u; z < kGridSize; ++z) {
            for (std::size_t y = 0u; y < kGridSize; ++y)
                line[y] = grid[gridIndex(x, y, z)];
            fftLine(line);
            for (std::size_t y = 0u; y < kGridSize; ++y)
                grid[gridIndex(x, y, z)] = line[y];
        }
    }
    for (std::size_t y = 0u; y < kGridSize; ++y) {
        for (std::size_t z = 0u; z < kGridSize; ++z) {
            for (std::size_t x = 0u; x < kGridSize; ++x)
                line[x] = grid[gridIndex(x, y, z)];
            fftLine(line);
            for (std::size_t x = 0u; x < kGridSize; ++x)
                grid[gridIndex(x, y, z)] = line[x];
        }
    }
}

QColor spectrumColor(float normalized)
{
    const QColor stops[] = {
        QColor(3, 18, 58), QColor(17, 76, 160), QColor(26, 170, 190),
        QColor(246, 205, 71), QColor(239, 93, 35)};
    constexpr std::size_t stopCount = sizeof(stops) / sizeof(stops[0]);
    const float value = std::clamp(normalized, 0.0f, 1.0f);
    const float scaled = value * static_cast<float>(stopCount - 1u);
    const std::size_t lower = std::min(stopCount - 2u, static_cast<std::size_t>(scaled));
    const float local = scaled - static_cast<float>(lower);
    return QColor(
        static_cast<int>(stops[lower].red() + local * (stops[lower + 1u].red() - stops[lower].red())),
        static_cast<int>(stops[lower].green() + local * (stops[lower + 1u].green() - stops[lower].green())),
        static_cast<int>(stops[lower].blue() + local * (stops[lower + 1u].blue() - stops[lower].blue())));
}
} // namespace

SpectrumGraph::SpectrumGraph()
    : QWidget(nullptr), _sampledParticleCount(0u), _deltaRms(0.0f), _hasAnalysisAt(false)
{
    setObjectName("spectrumGraphWidget");
    setMinimumHeight(148);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void SpectrumGraph::clearSpectrum()
{
    _k.clear();
    _power.clear();
    _history.clear();
    _historyTimes.clear();
    _historySteps.clear();
    _sampledParticleCount = 0u;
    _deltaRms = 0.0f;
    _hasAnalysisAt = false;
    update();
}

void SpectrumGraph::setSnapshot(const std::vector<RenderParticle>& snapshot,
                                float simulationTime,
                                std::uint64_t step)
{
    _sampledParticleCount = snapshot.size();
    if (!_historySteps.empty() && step < _historySteps.back())
        clearSpectrum();
    _sampledParticleCount = snapshot.size();
    if (snapshot.size() < 2u) {
        update();
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (_hasAnalysisAt && now - _lastAnalysisAt < kAnalysisInterval) {
        update();
        return;
    }
    _lastAnalysisAt = now;
    _hasAnalysisAt = true;
    _k.clear();
    _power.clear();
    float minValue[3] = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max()};
    float maxValue[3] = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                         std::numeric_limits<float>::lowest()};
    for (const RenderParticle& particle : snapshot) {
        const float values[3] = {particle.x, particle.y, particle.z};
        for (int axis = 0; axis < 3; ++axis) {
            minValue[axis] = std::min(minValue[axis], values[axis]);
            maxValue[axis] = std::max(maxValue[axis], values[axis]);
        }
    }
    const float span[3] = {std::max(1.0e-5f, maxValue[0] - minValue[0]),
                           std::max(1.0e-5f, maxValue[1] - minValue[1]),
                           std::max(1.0e-5f, maxValue[2] - minValue[2])};
    std::vector<float> density(kGridSize * kGridSize * kGridSize, 0.0f);
    float totalMass = 0.0f;
    for (const RenderParticle& particle : snapshot) {
        const float values[3] = {particle.x, particle.y, particle.z};
        std::size_t base[3] = {};
        float fraction[3] = {};
        for (int axis = 0; axis < 3; ++axis) {
            const float normalized = std::clamp(
                (values[axis] - minValue[axis]) / span[axis], 0.0f, 1.0f);
            const float position = normalized * static_cast<float>(kGridSize - 1u);
            base[axis] = std::min(kGridSize - 1u,
                                  static_cast<std::size_t>(std::floor(position)));
            fraction[axis] = position - static_cast<float>(base[axis]);
        }
        const float mass = std::max(0.0f, particle.mass);
        for (std::size_t xOffset = 0u; xOffset < 2u; ++xOffset) {
            const std::size_t x = std::min(kGridSize - 1u, base[0] + xOffset);
            const float xWeight = xOffset == 0u ? 1.0f - fraction[0] : fraction[0];
            for (std::size_t yOffset = 0u; yOffset < 2u; ++yOffset) {
                const std::size_t y = std::min(kGridSize - 1u, base[1] + yOffset);
                const float yWeight = yOffset == 0u ? 1.0f - fraction[1] : fraction[1];
                for (std::size_t zOffset = 0u; zOffset < 2u; ++zOffset) {
                    const std::size_t z = std::min(kGridSize - 1u, base[2] + zOffset);
                    const float zWeight = zOffset == 0u ? 1.0f - fraction[2] : fraction[2];
                    density[gridIndex(x, y, z)] += mass * xWeight * yWeight * zWeight;
                }
            }
        }
        totalMass += mass;
    }
    const float mean = totalMass / static_cast<float>(density.size());
    if (mean <= 0.0f) {
        update();
        return;
    }
    std::vector<Complex> transformed(density.size());
    float variance = 0.0f;
    for (std::size_t index = 0u; index < density.size(); ++index) {
        const float delta = density[index] / mean - 1.0f;
        transformed[index] = Complex(delta, 0.0f);
        variance += delta * delta;
    }
    _deltaRms = std::sqrt(variance / static_cast<float>(density.size()));
    transformGrid(transformed);
    std::vector<float> power(kSpectrumBins, 0.0f);
    std::vector<std::size_t> counts(kSpectrumBins, 0u);
    const float maxFrequency = std::sqrt(3.0f) * static_cast<float>(kGridSize / 2u);
    for (std::size_t x = 0u; x < kGridSize; ++x) {
        for (std::size_t y = 0u; y < kGridSize; ++y) {
            for (std::size_t z = 0u; z < kGridSize; ++z) {
                const int wrappedGridSize = static_cast<int>(kGridSize);
                const int fx = x <= kGridSize / 2u ? static_cast<int>(x) : static_cast<int>(x) - wrappedGridSize;
                const int fy = y <= kGridSize / 2u ? static_cast<int>(y) : static_cast<int>(y) - wrappedGridSize;
                const int fz = z <= kGridSize / 2u ? static_cast<int>(z) : static_cast<int>(z) - wrappedGridSize;
                const float frequency = std::sqrt(static_cast<float>(fx * fx + fy * fy + fz * fz));
                if (frequency <= 0.0f)
                    continue;
                const std::size_t bin = std::min(kSpectrumBins - 1u, static_cast<std::size_t>(
                    frequency / maxFrequency * static_cast<float>(kSpectrumBins)));
                power[bin] += std::norm(transformed[gridIndex(x, y, z)]) /
                              static_cast<float>(transformed.size());
                counts[bin] += 1u;
            }
        }
    }
    _k.clear();
    for (std::size_t bin = 0u; bin < kSpectrumBins; ++bin) {
        if (counts[bin] == 0u)
            continue;
        _k.push_back((static_cast<float>(bin) + 0.5f) * maxFrequency /
                     static_cast<float>(kSpectrumBins));
        _power.push_back(power[bin] / static_cast<float>(counts[bin]));
    }
    if (_historySteps.empty() || step > _historySteps.back()) {
        _history.push_back(_power);
        _historyTimes.push_back(simulationTime);
        _historySteps.push_back(step);
        if (_history.size() > kHistoryColumns) {
            _history.erase(_history.begin());
            _historyTimes.erase(_historyTimes.begin());
            _historySteps.erase(_historySteps.begin());
        }
    }
    else if (step == _historySteps.back()) {
        _history.back() = _power;
        _historyTimes.back() = simulationTime;
    }
    else {
        clearSpectrum();
        _history.push_back(_power);
        _historyTimes.push_back(simulationTime);
        _historySteps.push_back(step);
    }
    update();
}

std::size_t SpectrumGraph::sampledParticleCount() const
{
    return _sampledParticleCount;
}

void SpectrumGraph::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    const QColor background = palette().color(QPalette::Window);
    const QColor text = palette().color(QPalette::WindowText);
    const QColor grid = palette().color(QPalette::Mid);
    painter.fillRect(rect(), background);
    painter.setPen(grid);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
    const QRectF plot = rect().adjusted(62, 42, -14, -38);
    painter.drawRect(plot);
    painter.setPen(text);
    painter.drawText(QRectF(10, 7, width() - 20, 16), Qt::AlignLeft,
                     QStringLiteral("Correlation spectrogram  |  CIC P(k) density correlation  |  GUI sample %1  |  frames %2  |  delta RMS %3")
                         .arg(static_cast<qulonglong>(_sampledParticleCount))
                         .arg(static_cast<qulonglong>(_history.size()))
                         .arg(_deltaRms, 0, 'g', 4));
    const QRectF legend(width() - 230, 8, 105, 10);
    for (int index = 0; index < static_cast<int>(legend.width()); ++index) {
        const float normalized = static_cast<float>(index) /
                                 static_cast<float>(std::max(1, static_cast<int>(legend.width()) - 1));
        painter.fillRect(QRectF(legend.left() + index, legend.top(), 1.0, legend.height()),
                         spectrumColor(normalized));
    }
    painter.setPen(text);
    painter.drawText(QRectF(legend.left() - 82, 5, 78, 16), Qt::AlignRight,
                     QStringLiteral("homogeneous"));
    painter.drawText(QRectF(legend.right() + 4, 5, 92, 16), Qt::AlignLeft,
                     QStringLiteral("correlated"));
    if (_history.empty() || _power.empty()) {
        painter.drawText(plot, Qt::AlignCenter, QStringLiteral("Waiting for particle snapshot"));
        return;
    }
    constexpr float minPower = 1.0e-12f;
    float maxLogPower = std::log10(minPower);
    for (const std::vector<float>& row : _history) {
        for (float value : row)
            maxLogPower = std::max(maxLogPower, std::log10(std::max(value, minPower)));
    }
    constexpr float kColorRangeDb = 24.0f;
    const float minLogPower = maxLogPower - kColorRangeDb / 10.0f;
    const float logRange = std::max(1.0f, maxLogPower - minLogPower);
    const float columnWidth = static_cast<float>(plot.width()) /
                              static_cast<float>(std::max<std::size_t>(1u, _history.size()));
    const float binHeight = static_cast<float>(plot.height()) /
                            static_cast<float>(kSpectrumBins);
    for (std::size_t timeIndex = 0u; timeIndex < _history.size(); ++timeIndex) {
        const float x = static_cast<float>(plot.left()) +
                        static_cast<float>(timeIndex) * columnWidth;
        for (std::size_t bin = 0u; bin < _history[timeIndex].size(); ++bin) {
            const float logPower = std::log10(std::max(_history[timeIndex][bin], minPower));
            const float normalized = (logPower - minLogPower) / logRange;
            const float y = static_cast<float>(plot.bottom()) -
                            static_cast<float>(bin + 1u) * binHeight;
            painter.fillRect(QRectF(x, y, columnWidth + 0.5f, binHeight + 0.5f),
                             spectrumColor(normalized));
        }
    }
    painter.setPen(text);
    painter.drawText(QRectF(2, plot.top() - 2, 56, 18), Qt::AlignRight,
                     QStringLiteral("high k"));
    painter.drawText(QRectF(2, plot.bottom() - 16, 56, 18), Qt::AlignRight,
                     QStringLiteral("low k"));
    painter.drawText(QRectF(plot.left(), plot.bottom() + 5, 100, 18), Qt::AlignLeft,
                     QStringLiteral("t=%1").arg(_historyTimes.front(), 0, 'g', 3));
    painter.drawText(QRectF(plot.right() - 100, plot.bottom() + 5, 100, 18), Qt::AlignRight,
                     QStringLiteral("t=%1").arg(_historyTimes.back(), 0, 'g', 3));
}
} // namespace bltzr_qt
