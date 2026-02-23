#include "ui/ParticleView.hpp"

#include <QColor>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QSizePolicy>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace qtui {
namespace {
constexpr int kTempBins = 256;
constexpr int kPressureBins = 256;
constexpr float kTemperatureScaleFloor = 0.25f;
constexpr float kPressureScaleFloor = 0.25f;

float saturate(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

QColor blendColor(const QColor &a, const QColor &b, float t, int alpha)
{
    const float tt = saturate(t);
    return QColor(
        static_cast<int>(a.red() + (b.red() - a.red()) * tt),
        static_cast<int>(a.green() + (b.green() - a.green()) * tt),
        static_cast<int>(a.blue() + (b.blue() - a.blue()) * tt),
        alpha
    );
}

struct RgbColor {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

std::array<RgbColor, kTempBins> buildTemperatureLut()
{
    std::array<RgbColor, kTempBins> lut{};
    const QColor cold(56, 105, 255, 255);
    const QColor warm(255, 170, 90, 255);
    const QColor hot(255, 76, 66, 255);
    for (int i = 0; i < kTempBins; ++i) {
        const float tNorm = static_cast<float>(i) / static_cast<float>(kTempBins - 1);
        QColor c;
        if (tNorm < 0.55f) {
            c = blendColor(cold, warm, tNorm / 0.55f, 255);
        } else {
            c = blendColor(warm, hot, (tNorm - 0.55f) / 0.45f, 255);
        }
        lut[static_cast<std::size_t>(i)] = RgbColor{
            static_cast<std::uint8_t>(c.red()),
            static_cast<std::uint8_t>(c.green()),
            static_cast<std::uint8_t>(c.blue())
        };
    }
    return lut;
}

int quantizeToBin(float value, float rangeMax, int bins)
{
    if (value <= 0.0f) {
        return 0;
    }
    const float normalized = std::min(value / rangeMax, 1.0f);
    return static_cast<int>(normalized * static_cast<float>(bins - 1));
}

float updateAdaptiveScale(float current, float observed, float floorValue, float riseRate, float fallRate)
{
    const float target = std::max(floorValue, observed);
    const float rate = (target > current) ? riseRate : fallRate;
    return current + (target - current) * std::clamp(rate, 0.0f, 1.0f);
}

std::array<std::uint8_t, kPressureBins> buildAlphaLut(int luminosity)
{
    std::array<std::uint8_t, kPressureBins> lut{};
    const int clampedLum = std::clamp(luminosity, 0, 255);
    for (int i = 0; i < kPressureBins; ++i) {
        const float pNorm = static_cast<float>(i) / static_cast<float>(kPressureBins - 1);
        lut[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(clampedLum * (0.2f + 0.8f * pNorm)), 6, 255));
    }
    return lut;
}

QRgb particleRampColorFast(
    const RenderParticle &particle,
    float temperatureScale,
    float pressureScale,
    const std::array<RgbColor, kTempBins> &tempLut,
    const std::array<std::uint8_t, kPressureBins> &alphaLut
)
{
    const int tIdx = quantizeToBin(particle.temperature, std::max(kTemperatureScaleFloor, temperatureScale), kTempBins);
    const int pIdx = quantizeToBin(particle.pressureNorm, std::max(kPressureScaleFloor, pressureScale), kPressureBins);
    const RgbColor c = tempLut[static_cast<std::size_t>(tIdx)];
    const std::uint8_t alpha = alphaLut[static_cast<std::size_t>(pIdx)];
    return qRgba(c.r, c.g, c.b, alpha);
}
} // namespace

ParticleView::ParticleView(ViewMode mode)
    : QWidget(nullptr),
      _mode(mode),
      _snapshot(std::nullopt),
      _zoom(8.0f),
      _luminosity(100),
      _camera{0.0f, 0.0f, 0.0f},
      _adaptiveTemperatureScale(2.0f),
      _adaptivePressureScale(2.0f),
      _dragAxis(GimbalAxis::None)
{
    setMinimumSize(220, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ParticleView::setSnapshot(const std::vector<RenderParticle> &snapshot)
{
    _snapshot = std::cref(snapshot);
    update();
}

void ParticleView::setMode(ViewMode mode)
{
    _mode = mode;
    update();
}

void ParticleView::setZoom(float zoom)
{
    _zoom = std::max(0.1f, zoom);
    update();
}

void ParticleView::setLuminosity(int luminosity)
{
    _luminosity = std::clamp(luminosity, 0, 255);
    update();
}

void ParticleView::setCameraAngles(float yaw, float pitch, float roll)
{
    _camera.yaw = yaw;
    _camera.pitch = pitch;
    _camera.roll = roll;
    update();
}

void ParticleView::mousePressEvent(MouseEventHandle event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    const GimbalOverlay gimbal = computeGimbal(rect(), _mode, _camera);
    if (!gimbal.rect.contains(event->position())) {
        QWidget::mousePressEvent(event);
        return;
    }

    const GimbalAxis axis = pickGimbalAxis(gimbal, event->position());
    if (axis == GimbalAxis::None) {
        QWidget::mousePressEvent(event);
        return;
    }

    _dragAxis = axis;
    _lastMousePos = event->position();
    event->accept();
}

void ParticleView::mouseMoveEvent(MouseEventHandle event)
{
    if (_dragAxis == GimbalAxis::None) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPointF pos = event->position();
    const float dx = static_cast<float>(pos.x() - _lastMousePos.x());
    const float dy = static_cast<float>(pos.y() - _lastMousePos.y());
    const float delta = (dx - dy) * 0.01f;

    if (_dragAxis == GimbalAxis::X) {
        _camera.pitch += delta;
    } else if (_dragAxis == GimbalAxis::Y) {
        _camera.yaw += delta;
    } else if (_dragAxis == GimbalAxis::Z) {
        _camera.roll += delta;
    }
    _lastMousePos = pos;
    update();
    event->accept();
}

void ParticleView::mouseReleaseEvent(MouseEventHandle event)
{
    _dragAxis = GimbalAxis::None;
    QWidget::mouseReleaseEvent(event);
}

void ParticleView::paintEvent(PaintEventHandle event)
{
    (void)event;
    if (_framebuffer.size() != size()) {
        _framebuffer = QImage(size(), QImage::Format_ARGB32);
    }
    _framebuffer.fill(QColor(10, 10, 16));

    const int widthPx = _framebuffer.width();
    const int heightPx = _framebuffer.height();
    const float centerX = width() * 0.5f;
    const float centerY = height() * 0.5f;
    static const std::array<RgbColor, kTempBins> temperatureLut = buildTemperatureLut();
    const std::array<std::uint8_t, kPressureBins> alphaLut = buildAlphaLut(_luminosity);
    const QRgb heavyBodyColor = qRgba(255, 90, 90, static_cast<std::uint8_t>(_luminosity));

    if (_snapshot.has_value()) {
        const std::vector<RenderParticle> &snapshot = _snapshot->get();
        float observedTempMax = kTemperatureScaleFloor;
        float observedPressureMax = kPressureScaleFloor;
        for (const RenderParticle &particle : snapshot) {
            observedTempMax = std::max(observedTempMax, std::max(0.0f, particle.temperature));
            observedPressureMax = std::max(observedPressureMax, std::max(0.0f, particle.pressureNorm));
        }
        _adaptiveTemperatureScale = updateAdaptiveScale(_adaptiveTemperatureScale, observedTempMax, kTemperatureScaleFloor, 0.32f, 0.04f);
        _adaptivePressureScale = updateAdaptiveScale(_adaptivePressureScale, observedPressureMax, kPressureScaleFloor, 0.32f, 0.04f);

        for (const RenderParticle &particle : snapshot) {
            const ProjectedPoint pp = projectParticle(particle, _mode, _camera);
            if (!pp.valid) {
                continue;
            }

            const int x = static_cast<int>(centerX + pp.x * _zoom);
            const int y = static_cast<int>(centerY - pp.y * _zoom);
            if (x < 0 || x >= widthPx || y < 0 || y >= heightPx) {
                continue;
            }

            QRgb color = 0;
            if (particle.mass > 100.0f) {
                color = heavyBodyColor;
            } else {
                color = particleRampColorFast(
                    particle,
                    _adaptiveTemperatureScale,
                    _adaptivePressureScale,
                    temperatureLut,
                    alphaLut
                );
            }

            _framebuffer.setPixel(x, y, color);
        }
    }

    QPainter painter(this);
    painter.drawImage(0, 0, _framebuffer);

    const GimbalOverlay gimbal = computeGimbal(rect(), _mode, _camera);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(120, 120, 135), 1.0));
    painter.setBrush(QColor(18, 18, 26, 180));
    painter.drawEllipse(gimbal.rect);

    const std::array<QColor, 3> colors = {
        QColor(255, 95, 95),
        QColor(85, 255, 125),
        QColor(105, 150, 255)
    };
    for (std::size_t i = 0; i < gimbal.handles.size(); ++i) {
        painter.setPen(QPen(colors[i], 1.6));
        painter.drawLine(gimbal.center, gimbal.handles[i]);
        painter.setBrush(colors[i]);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(gimbal.handles[i], 3.6, 3.6);
    }
    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.setPen(QColor(48, 48, 60));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

} // namespace qtui
