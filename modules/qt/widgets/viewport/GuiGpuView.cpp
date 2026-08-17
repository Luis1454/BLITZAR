/*
 * @file modules/qt/widgets/viewport/GuiGpuView.cpp
 * @brief OpenGL point-sprite renderer for one synchronized particle view.
 */

#include "widgets/viewport/GuiGpuView.hpp"
#include "core/constants/FndConstants.hpp"
#include "graphics/color/GfxColorPipeline.hpp"
#include "widgets/overlays/GuiPainter.hpp"
#include "widgets/viewport/GuiShaderSources.hpp"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPainter>
#include <QPen>
#include <QShowEvent>
#include <QTimer>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>

namespace bltzr_qt {
int modeValue(grav::ViewMode mode)
{
    return static_cast<int>(mode);
}

GpuView::GpuView(grav::ViewMode mode, QWidget* parent)
    : QOpenGLWidget(parent),
      _mode(mode),
      _vertices(),
      _buffer(QOpenGLBuffer::VertexBuffer),
      _program(),
      _vao(),
      _unavailableCallback(),
      _overlay(std::nullopt),
      _overlayEnabled(false),
      _overlayOpacity(kOverlayOpacityDefault),
      _pendingUpload(false),
      _ready(false),
      _reportedUnavailable(false),
      _uploadedCount(0u),
      _frameCount(0u),
      _lastFrameMs(0.0f),
      _lastUploadMs(0.0f),
      _zoom(kDefaultZoom),
      _luminosity(kDefaultLuminosity),
      _camera{0.0f, 0.0f, 0.0f},
      _adaptiveTemperatureScale(2.0f),
      _adaptivePressureScale(2.0f),
      _cullingEnabled(true),
      _lodEnabled(true),
      _lodNearDistance(kRenderLODNearDistance),
      _lodFarDistance(kRenderLODFarDistance),
      _dragAxis(grav::GimbalAxis::None),
      _lastMousePos()
{
    setMinimumSize(220, 180);
    setAutoFillBackground(false);
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
}

GpuView::~GpuView()
{
    if (context()) {
        makeCurrent();
        _vao.destroy();
        _buffer.destroy();
        _program.reset();
        doneCurrent();
    }
}

void GpuView::setSnapshot(const std::vector<RenderParticle>& snapshot)
{
    _vertices.clear();
    _vertices.reserve(snapshot.size());
    for (const RenderParticle& particle : snapshot) {
        _vertices.push_back(Vertex{particle.x, particle.y, particle.z, particle.mass,
                                   particle.pressureNorm, particle.temperature,
                                   particle.densityNorm});
    }
    grav::updateAdaptiveScales(snapshot, _adaptiveTemperatureScale, _adaptivePressureScale);
    _pendingUpload = true;
    update();
}

void GpuView::setMode(grav::ViewMode mode)
{
    _mode = mode;
    update();
}

void GpuView::setZoom(float zoom)
{
    _zoom = std::max(kViewportMinZoom, zoom);
    update();
}

void GpuView::setLuminosity(int luminosity)
{
    _luminosity = std::clamp(luminosity, kLuminosityMin, kLuminosityMax);
    update();
}

void GpuView::setCameraAngles(float yaw, float pitch, float roll)
{
    _camera = grav::CameraState{yaw, pitch, roll};
    update();
}

void GpuView::setRenderSettings(bool culling, bool lod, float nearDist, float farDist)
{
    _cullingEnabled = culling;
    _lodEnabled = lod;
    _lodNearDistance = nearDist;
    _lodFarDistance = std::max(nearDist + 0.001f, farDist);
    update();
}

void GpuView::setOctreeOverlay(const std::vector<OctreeNode>& overlay, bool enabled, int opacity)
{
    _overlay = std::cref(overlay);
    _overlayEnabled = enabled;
    _overlayOpacity = std::clamp(opacity, kLuminosityMin, kLuminosityMax);
    update();
}

void GpuView::setUnavailableCallback(std::function<void()> callback)
{
    _unavailableCallback = std::move(callback);
}

bool GpuView::isReady() const
{
    return _ready;
}

GpuViewMetrics GpuView::metrics() const
{
    return GpuViewMetrics{_frameCount, _uploadedCount, _lastFrameMs, _lastUploadMs};
}

void GpuView::initializeGL()
{
    const QSurfaceFormat format = context()->format();
    if (format.majorVersion() < 3 || (format.majorVersion() == 3 && format.minorVersion() < 3)) {
        std::cerr << "[viewport] OpenGL unavailable version=" << format.majorVersion() << "."
                  << format.minorVersion() << "\n";
        reportUnavailable();
        return;
    }
    _program = std::make_unique<QOpenGLShaderProgram>();
    if (!_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource()) ||
        !_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource()) ||
        !_program->link()) {
        std::cerr << "[viewport] OpenGL shader failure: " << _program->log().toStdString() << "\n";
        _program.reset();
        reportUnavailable();
        return;
    }
    if (!_vao.create() || !_buffer.create()) {
        reportUnavailable();
        return;
    }
    QOpenGLFunctions* functions = context()->functions();
    functions->glEnable(GL_PROGRAM_POINT_SIZE);
    _ready = true;
    std::clog << "[viewport] OpenGL renderer ready version=" << format.majorVersion() << "."
              << format.minorVersion() << "\n";
}

void GpuView::uploadPendingSnapshot()
{
    if (!_pendingUpload || !_ready || !_program || !_buffer.isCreated()) {
        return;
    }
    const auto uploadStart = std::chrono::steady_clock::now();
    _vao.bind();
    _buffer.bind();
    const int bytes = static_cast<int>(_vertices.size() * sizeof(Vertex));
    _buffer.allocate(_vertices.empty() ? nullptr : _vertices.data(), bytes);
    _program->bind();
    _program->enableAttributeArray(0);
    _program->enableAttributeArray(1);
    _program->enableAttributeArray(2);
    _program->enableAttributeArray(3);
    _program->enableAttributeArray(4);
    _program->setAttributeBuffer(0, GL_FLOAT, offsetof(Vertex, x), 3, sizeof(Vertex));
    _program->setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, mass), 1, sizeof(Vertex));
    _program->setAttributeBuffer(2, GL_FLOAT, offsetof(Vertex, pressure), 1, sizeof(Vertex));
    _program->setAttributeBuffer(3, GL_FLOAT, offsetof(Vertex, temperature), 1, sizeof(Vertex));
    _program->setAttributeBuffer(4, GL_FLOAT, offsetof(Vertex, density), 1, sizeof(Vertex));
    _program->release();
    _buffer.release();
    _vao.release();
    _uploadedCount = _vertices.size();
    _pendingUpload = false;
    _lastUploadMs =
        std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - uploadStart)
            .count();
}

void GpuView::paintGL()
{
    const auto frameStart = std::chrono::steady_clock::now();
    QOpenGLFunctions* functions = context()->functions();
    functions->glClearColor(0.039f, 0.039f, 0.063f, 1.0f);
    functions->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (_ready && _program) {
        uploadPendingSnapshot();
        _program->bind();
        _program->setUniformValue("u_mode", modeValue(_mode));
        _program->setUniformValue("u_camera", _camera.yaw, _camera.pitch, _camera.roll);
        _program->setUniformValue("u_zoom", _zoom);
        _program->setUniformValue("u_point_scale", zoomCompensationLambda(_zoom));
        _program->setUniformValue("u_viewport", static_cast<float>(width()),
                                  static_cast<float>(height()));
        _program->setUniformValue("u_lod_near", _lodNearDistance);
        _program->setUniformValue("u_lod_far", _lodFarDistance);
        _program->setUniformValue("u_lod_enabled", _lodEnabled);
        _program->setUniformValue("u_temperature_scale", _adaptiveTemperatureScale);
        _program->setUniformValue("u_pressure_scale", _adaptivePressureScale);
        _program->setUniformValue("u_luminosity", static_cast<float>(_luminosity) / 255.0f);
        _vao.bind();
        functions->glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(_uploadedCount));
        _vao.release();
        _program->release();
    }
    paintOverlay();
    _frameCount += 1u;
    _lastFrameMs =
        std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - frameStart)
            .count();
}

void GpuView::resizeGL(int width, int height)
{
    context()->functions()->glViewport(0, 0, width, height);
}

void GpuView::showEvent(QShowEvent* event)
{
    QOpenGLWidget::showEvent(event);
    QTimer::singleShot(250, this, [this]() {
        if (!_ready && context() == nullptr) {
            std::cerr << "[viewport] OpenGL context creation failed\n";
            reportUnavailable();
        }
    });
}

void GpuView::paintOverlay()
{
    QPainter painter(this);
    if (_overlayEnabled && _overlay.has_value()) {
        paintOctreeOverlay(painter, rect(), _mode, _camera, _overlay->get(), _zoom,
                           _overlayOpacity);
    }
    const GimbalOverlay gimbal = computeGimbal(rect(), _mode, _camera);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(120, 120, 135), 1.0));
    painter.setBrush(QColor(18, 18, 26, 180));
    painter.drawEllipse(gimbal.rect);
    const std::array<QColor, 3> colors = {QColor(255, 95, 95), QColor(85, 255, 125),
                                          QColor(105, 150, 255)};
    for (std::size_t index = 0; index < gimbal.handles.size(); ++index) {
        painter.setPen(QPen(colors[index], 1.6));
        painter.drawLine(gimbal.center, gimbal.handles[index]);
        painter.setBrush(colors[index]);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(gimbal.handles[index], 3.6, 3.6);
    }
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QColor(48, 48, 60));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void GpuView::reportUnavailable()
{
    if (_reportedUnavailable) {
        return;
    }
    _reportedUnavailable = true;
    QTimer::singleShot(0, this, [this]() {
        if (_unavailableCallback) {
            _unavailableCallback();
        }
    });
}

} // namespace bltzr_qt
