/*
 * @file modules/qt/src/widgets/viewport/GpuView.cpp
 * @brief OpenGL point-sprite renderer for one synchronized particle view.
 */

#include "widgets/viewport/GpuView.hpp"
#include "graphics/ColorPipeline.hpp"
#include "widgets/overlays/Painter.hpp"
#include "Constants.hpp"
#include <QMouseEvent>
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
namespace {
constexpr char kVertexShader[] = R"GLSL(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in float a_mass;
layout(location = 2) in float a_pressure;
layout(location = 3) in float a_temperature;
layout(location = 4) in float a_density;
uniform int u_mode;
uniform vec3 u_camera;
uniform float u_zoom;
uniform float u_point_scale;
uniform vec2 u_viewport;
uniform float u_lod_near;
uniform float u_lod_far;
uniform bool u_lod_enabled;
out float v_pressure;
out float v_temperature;
out float v_mass;
out float v_density;
void rotate3d(vec3 value, vec3 angles, out vec3 result) {
    float cy = cos(angles.x);
    float sy = sin(angles.x);
    float cp = cos(angles.y);
    float sp = sin(angles.y);
    float cr = cos(angles.z);
    float sr = sin(angles.z);
    float x1 = cy * value.x - sy * value.z;
    float z1 = sy * value.x + cy * value.z;
    float y1 = cp * value.y - sp * z1;
    float z2 = sp * value.y + cp * z1;
    result = vec3(cr * x1 - sr * y1, sr * x1 + cr * y1, z2);
}
void main() {
    vec3 base = (u_mode >= 3) ? vec3(0.7853981634, 0.6154797087, 0.0) : vec3(0.0);
    vec3 rotated;
    rotate3d(a_position, base + u_camera, rotated);
    float sx = rotated.x;
    float sy = rotated.y;
    float depth = rotated.z;
    if (u_mode == 1) { sy = rotated.z; depth = rotated.y; }
    if (u_mode == 2) { sx = rotated.y; sy = rotated.z; depth = rotated.x; }
    if (u_mode == 4) {
        float denominator = 40.0 - depth;
        float perspective = 40.0 / denominator;
        if (abs(denominator) < 0.001 || perspective <= 0.0 || perspective > 30.0) {
            gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
            return;
        }
        sx *= perspective;
        sy *= perspective;
    }
    float distanceFromOrigin = length(a_position);
    if (u_lod_enabled && u_mode == 4 && distanceFromOrigin > u_lod_near &&
        (uint(gl_VertexID) % 100u) < uint(clamp((distanceFromOrigin - u_lod_near) /
        max(0.001, u_lod_far - u_lod_near), 0.0, 1.0) * 100.0)) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        return;
    }
    vec2 halfViewport = max(u_viewport * 0.5, vec2(1.0));
    gl_Position = vec4((sx * u_zoom) / halfViewport.x,
                       (sy * u_zoom) / halfViewport.y, clamp(depth / 40.0, -1.0, 1.0), 1.0);
    gl_PointSize = clamp((a_mass > 100.0 ? 2.0 : 1.0) * u_point_scale, 1.0, 32.0);
    v_pressure = a_pressure;
    v_temperature = a_temperature;
    v_mass = a_mass;
    v_density = a_density;
}
)GLSL";

constexpr char kFragmentShader[] = R"GLSL(
#version 330 core
in float v_pressure;
in float v_temperature;
in float v_mass;
in float v_density;
uniform float u_temperature_scale;
uniform float u_pressure_scale;
uniform float u_luminosity;
out vec4 fragColor;
void main() {
    if (v_mass > 100.0) {
        fragColor = vec4(1.0, 0.35, 0.35, u_luminosity > 0.0 ? 1.0 : 0.0);
        return;
    }
    float temperature = clamp(v_temperature / max(0.25, u_temperature_scale), 0.0, 1.0);
    vec3 cold = vec3(0.22, 0.41, 1.0);
    vec3 warm = vec3(1.0, 0.67, 0.35);
    vec3 hot = vec3(1.0, 0.30, 0.26);
    vec3 color = temperature < 0.55
        ? mix(cold, warm, temperature / 0.55)
        : mix(warm, hot, (temperature - 0.55) / 0.45);
    float pressure = clamp(v_pressure / max(0.25, u_pressure_scale), 0.0, 1.0);
    float density = clamp(v_density, 0.0, 1.0);
    if (density > 0.0) {
        vec3 densityColor = mix(vec3(0.14, 0.29, 0.75), vec3(1.0, 0.88, 0.35), density);
        color = mix(color, densityColor, 0.65);
    }
    color *= 0.35 + 0.65 * density;
    float visibility = max(0.55, max(pressure, density));
    fragColor = vec4(color, u_luminosity > 0.0 ? 1.0 : 0.0);
}
)GLSL";

int modeValue(grav::ViewMode mode)
{
    return static_cast<int>(mode);
}
} // namespace

GpuView::GpuView(grav::ViewMode mode)
    : QOpenGLWidget(nullptr),
      _mode(mode),
      _vertices(),
      _buffer(QOpenGLBuffer::VertexBuffer),
      _program(),
      _vao(),
      _unavailableCallback(),
      _overlay(nullptr),
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
    _overlay = &overlay;
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
    if (!_program->addShaderFromSourceCode(QOpenGLShader::Vertex, kVertexShader) ||
        !_program->addShaderFromSourceCode(QOpenGLShader::Fragment, kFragmentShader) ||
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
    _lastUploadMs = std::chrono::duration<float, std::milli>(
                         std::chrono::steady_clock::now() - uploadStart)
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
    _lastFrameMs = std::chrono::duration<float, std::milli>(
                       std::chrono::steady_clock::now() - frameStart)
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
    if (_overlayEnabled && _overlay != nullptr) {
        paintOctreeOverlay(painter, rect(), _mode, _camera, *_overlay, _zoom, _overlayOpacity);
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

void GpuView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }
    const GimbalOverlay gimbal = computeGimbal(rect(), _mode, _camera);
    if (!gimbal.rect.contains(event->position())) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }
    _dragAxis = pickGimbalAxis(gimbal, event->position());
    if (_dragAxis == grav::GimbalAxis::None) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }
    _lastMousePos = event->position();
    event->accept();
}

void GpuView::mouseMoveEvent(QMouseEvent* event)
{
    if (_dragAxis == grav::GimbalAxis::None) {
        QOpenGLWidget::mouseMoveEvent(event);
        return;
    }
    const QPointF position = event->position();
    const float dx = static_cast<float>(position.x() - _lastMousePos.x());
    const float dy = static_cast<float>(position.y() - _lastMousePos.y());
    if (_dragAxis == grav::GimbalAxis::X) {
        _camera.yaw += dx * 0.5f;
    }
    else if (_dragAxis == grav::GimbalAxis::Y) {
        _camera.pitch -= dy * 0.5f;
    }
    else {
        _camera.roll += dx * 0.5f;
    }
    _lastMousePos = position;
    update();
    event->accept();
}

void GpuView::mouseReleaseEvent(QMouseEvent* event)
{
    if (_dragAxis != grav::GimbalAxis::None) {
        _dragAxis = grav::GimbalAxis::None;
        event->accept();
        return;
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}
} // namespace bltzr_qt
