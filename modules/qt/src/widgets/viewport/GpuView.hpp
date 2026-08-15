/*
 * @file modules/qt/src/widgets/viewport/GpuView.hpp
 * @brief OpenGL point-sprite renderer for one synchronized particle view.
 */

#ifndef BLITZAR_MODULES_QT_SRC_WIDGETS_VIEWPORT_GPUVIEW_HPP_
#define BLITZAR_MODULES_QT_SRC_WIDGETS_VIEWPORT_GPUVIEW_HPP_

#include "Constants.hpp"
#include "widgets/overlays/Octree.hpp"
#include "support/geometry/ViewMath.hpp"
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPointF>
#include <functional>
#include <memory>
#include <vector>

class QMouseEvent;
class QPaintEvent;
class QShowEvent;

namespace bltzr_qt {
struct GpuViewMetrics {
    std::size_t frameCount;
    std::size_t uploadedPoints;
    float lastFrameMs;
    float lastUploadMs;
};

class GpuView final : public QOpenGLWidget {
public:
    explicit GpuView(grav::ViewMode mode);
    ~GpuView() override;

    void setSnapshot(const std::vector<RenderParticle>& snapshot);
    void setMode(grav::ViewMode mode);
    void setZoom(float zoom);
    void setLuminosity(int luminosity);
    void setCameraAngles(float yaw, float pitch, float roll);
    void setRenderSettings(bool culling, bool lod, float nearDist, float farDist);
    void setOctreeOverlay(const std::vector<OctreeNode>& overlay, bool enabled, int opacity);
    void setUnavailableCallback(std::function<void()> callback);
    bool isReady() const;
    GpuViewMetrics metrics() const;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    struct Vertex {
        float x;
        float y;
        float z;
        float mass;
        float pressure;
        float temperature;
        float density;
    };

    void uploadPendingSnapshot();
    void paintOverlay();
    void reportUnavailable();
    grav::ViewMode _mode;
    std::vector<Vertex> _vertices;
    QOpenGLBuffer _buffer;
    std::unique_ptr<QOpenGLShaderProgram> _program;
    QOpenGLVertexArrayObject _vao;
    std::function<void()> _unavailableCallback;
    const std::vector<OctreeNode>* _overlay;
    bool _overlayEnabled;
    int _overlayOpacity;
    bool _pendingUpload;
    bool _ready;
    bool _reportedUnavailable;
    std::size_t _uploadedCount;
    std::size_t _frameCount;
    float _lastFrameMs;
    float _lastUploadMs;
    float _zoom;
    int _luminosity;
    grav::CameraState _camera;
    float _adaptiveTemperatureScale;
    float _adaptivePressureScale;
    bool _cullingEnabled;
    bool _lodEnabled;
    float _lodNearDistance;
    float _lodFarDistance;
    grav::GimbalAxis _dragAxis;
    QPointF _lastMousePos;
};
} // namespace bltzr_qt

#endif // BLITZAR_MODULES_QT_SRC_WIDGETS_VIEWPORT_GPUVIEW_HPP_
