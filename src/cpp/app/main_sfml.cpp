#include "sim/SimulationBackend.hpp"
#include "sim/SimulationArgs.hpp"
#include "sim/SimulationConfig.hpp"
#include "sim/SimulationInitConfig.hpp"

#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#endif

namespace {
enum class ViewMode {
    XY,
    XZ,
    YZ,
    Iso,
    Perspective
};

enum class GimbalAxis {
    None,
    X,
    Y,
    Z
};

struct ViewPanel {
    float x;
    float y;
    float width;
    float height;
    ViewMode mode;
};

struct RectF {
    float left;
    float top;
    float width;
    float height;
};

struct Layout {
    std::array<ViewPanel, 4> panels;
    RectF graphRect;
};

struct CameraState {
    float yaw;
    float pitch;
    float roll;
};

struct UiControls {
    double luminosity;
    float zoom;
    bool perspective3d;
    bool sphEnabled;
    float octreeTheta;
    float octreeSoftening;
    std::string exportFormat;
    std::string exportDirectory;
};

struct ProjectedPoint {
    float x;
    float y;
    float depth;
    bool valid;
};

struct EnergyPoint {
    float kinetic;
    float potential;
    float thermal;
    float radiated;
    float total;
    float drift;
};

struct GimbalOverlay {
    RectF rect;
    sf::Vector2f center;
    float radius;
    std::array<sf::Vector2f, 3> handles;
};

struct DragState {
    bool active;
    int panelIndex;
    GimbalAxis axis;
    sf::Vector2f lastPos;
};

constexpr float kIsoYaw = 0.78539816339f;
constexpr float kIsoPitch = 0.61547970867f;
constexpr std::size_t kEnergyHistoryMax = 720;
constexpr int kTempBins = 256;
constexpr int kPressureBins = 256;
constexpr float kTemperatureScaleFloor = 0.25f;
constexpr float kPressureScaleFloor = 0.25f;

float saturate(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

sf::Color lerpColor(const sf::Color &a, const sf::Color &b, float t, std::uint8_t alpha)
{
    const float tt = saturate(t);
    sf::Color color;
    color.r = static_cast<std::uint8_t>(a.r + (b.r - a.r) * tt);
    color.g = static_cast<std::uint8_t>(a.g + (b.g - a.g) * tt);
    color.b = static_cast<std::uint8_t>(a.b + (b.b - a.b) * tt);
    color.a = alpha;
    return color;
}

std::array<sf::Color, kTempBins> buildTemperatureLut()
{
    std::array<sf::Color, kTempBins> lut{};
    const sf::Color cold(56, 105, 255, 255);
    const sf::Color warm(255, 167, 78, 255);
    const sf::Color hot(255, 76, 66, 255);
    for (int i = 0; i < kTempBins; ++i) {
        const float tNorm = static_cast<float>(i) / static_cast<float>(kTempBins - 1);
        if (tNorm < 0.55f) {
            lut[static_cast<std::size_t>(i)] = lerpColor(cold, warm, tNorm / 0.55f, 255);
        } else {
            lut[static_cast<std::size_t>(i)] = lerpColor(warm, hot, (tNorm - 0.55f) / 0.45f, 255);
        }
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

std::array<std::uint8_t, kPressureBins> buildAlphaLut(double luminosity)
{
    std::array<std::uint8_t, kPressureBins> lut{};
    const double clampedLum = std::clamp(luminosity, 0.0, 255.0);
    for (int i = 0; i < kPressureBins; ++i) {
        const double pNorm = static_cast<double>(i) / static_cast<double>(kPressureBins - 1);
        lut[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(std::clamp(clampedLum * (0.25 + 0.75 * pNorm), 10.0, 255.0));
    }
    return lut;
}

sf::Color colorRampFast(
    float temperature,
    float pressureNorm,
    float temperatureScale,
    float pressureScale,
    const std::array<sf::Color, kTempBins> &tempLut,
    const std::array<std::uint8_t, kPressureBins> &alphaLut
)
{
    const int tIdx = quantizeToBin(temperature, std::max(kTemperatureScaleFloor, temperatureScale), kTempBins);
    const int pIdx = quantizeToBin(pressureNorm, std::max(kPressureScaleFloor, pressureScale), kPressureBins);
    sf::Color color = tempLut[static_cast<std::size_t>(tIdx)];
    color.a = alphaLut[static_cast<std::size_t>(pIdx)];
    return color;
}

void rotate3D(float x, float y, float z, float yaw, float pitch, float roll, float &outX, float &outY, float &outZ)
{
    const float cy = std::cos(yaw);
    const float sy = std::sin(yaw);
    const float cp = std::cos(pitch);
    const float sp = std::sin(pitch);
    const float cr = std::cos(roll);
    const float sr = std::sin(roll);

    const float x1 = cy * x - sy * z;
    const float z1 = sy * x + cy * z;
    const float y1 = cp * y - sp * z1;
    const float z2 = sp * y + cp * z1;

    outX = cr * x1 - sr * y1;
    outY = sr * x1 + cr * y1;
    outZ = z2;
}

void baseOrientation(ViewMode mode, float &yaw, float &pitch, float &roll)
{
    yaw = 0.0f;
    pitch = 0.0f;
    roll = 0.0f;
    if (mode == ViewMode::Iso || mode == ViewMode::Perspective) {
        yaw = kIsoYaw;
        pitch = kIsoPitch;
    }
}

void modeComponents(ViewMode mode, float x, float y, float z, float &sx, float &sy, float &depth)
{
    switch (mode) {
        case ViewMode::XY:
            sx = x;
            sy = y;
            depth = z;
            break;
        case ViewMode::XZ:
            sx = x;
            sy = z;
            depth = y;
            break;
        case ViewMode::YZ:
            sx = y;
            sy = z;
            depth = x;
            break;
        case ViewMode::Iso:
        case ViewMode::Perspective:
        default:
            sx = x;
            sy = y;
            depth = z;
            break;
    }
}

ProjectedPoint projectParticle(const RenderParticle &particle, ViewMode mode, const CameraState &camera)
{
    float baseYaw = 0.0f;
    float basePitch = 0.0f;
    float baseRoll = 0.0f;
    baseOrientation(mode, baseYaw, basePitch, baseRoll);

    float rx = 0.0f;
    float ry = 0.0f;
    float rz = 0.0f;
    rotate3D(
        particle.x,
        particle.y,
        particle.z,
        baseYaw + camera.yaw,
        basePitch + camera.pitch,
        baseRoll + camera.roll,
        rx,
        ry,
        rz
    );

    float sx = 0.0f;
    float sy = 0.0f;
    float depth = 0.0f;
    modeComponents(mode, rx, ry, rz, sx, sy, depth);

    if (mode != ViewMode::Perspective) {
        return ProjectedPoint{sx, sy, depth, true};
    }

    const float cameraDistance = 40.0f;
    const float denom = cameraDistance - depth;
    if (std::fabs(denom) < 1e-3f) {
        return ProjectedPoint{0.0f, 0.0f, depth, false};
    }
    const float perspectiveScale = cameraDistance / denom;
    if (perspectiveScale <= 0.0f || perspectiveScale > 30.0f) {
        return ProjectedPoint{0.0f, 0.0f, depth, false};
    }
    return ProjectedPoint{sx * perspectiveScale, sy * perspectiveScale, depth, true};
}

float clampGraphHeight(float windowHeight)
{
    const float minGraph = 100.0f;
    const float maxGraph = 260.0f;
    return std::clamp(windowHeight * 0.24f, minGraph, maxGraph);
}

Layout computeLayout(sf::Vector2u ws, bool perspective3d)
{
    const float w = static_cast<float>(ws.x);
    const float h = static_cast<float>(ws.y);
    const float margin = 8.0f;

    float graphH = clampGraphHeight(h);
    float viewAreaH = h - graphH - margin * 3.0f;
    if (viewAreaH < 220.0f) {
        graphH = std::max(70.0f, h * 0.16f);
        viewAreaH = h - graphH - margin * 3.0f;
    }

    const float panelW = std::max(10.0f, (w - margin * 3.0f) * 0.5f);
    const float panelH = std::max(10.0f, (viewAreaH - margin) * 0.5f);

    Layout layout{
        {
            ViewPanel{margin, margin, panelW, panelH, ViewMode::XY},
            ViewPanel{margin * 2.0f + panelW, margin, panelW, panelH, ViewMode::XZ},
            ViewPanel{margin, margin * 2.0f + panelH, panelW, panelH, ViewMode::YZ},
            ViewPanel{margin * 2.0f + panelW, margin * 2.0f + panelH, panelW, panelH, perspective3d ? ViewMode::Perspective : ViewMode::Iso}
        },
        RectF{margin, h - graphH - margin, std::max(10.0f, w - margin * 2.0f), graphH}
    };
    return layout;
}

GimbalOverlay computeGimbal(const ViewPanel &panel, ViewMode mode, const CameraState &camera)
{
    const float size = std::clamp(std::min(panel.width, panel.height) * 0.26f, 54.0f, 94.0f);
    const float margin = 8.0f;
    const RectF rect{panel.x + panel.width - size - margin, panel.y + margin, size, size};
    const sf::Vector2f center(rect.left + rect.width * 0.5f, rect.top + rect.height * 0.5f);
    const float radius = rect.width * 0.45f;

    float baseYaw = 0.0f;
    float basePitch = 0.0f;
    float baseRoll = 0.0f;
    baseOrientation(mode, baseYaw, basePitch, baseRoll);

    std::array<sf::Vector2f, 3> handles{};
    const std::array<std::array<float, 3>, 3> axes = {
        std::array<float, 3>{1.0f, 0.0f, 0.0f},
        std::array<float, 3>{0.0f, 1.0f, 0.0f},
        std::array<float, 3>{0.0f, 0.0f, 1.0f}
    };

    for (std::size_t i = 0; i < axes.size(); ++i) {
        float rx = 0.0f;
        float ry = 0.0f;
        float rz = 0.0f;
        rotate3D(
            axes[i][0],
            axes[i][1],
            axes[i][2],
            baseYaw + camera.yaw,
            basePitch + camera.pitch,
            baseRoll + camera.roll,
            rx,
            ry,
            rz
        );

        float sx = 0.0f;
        float sy = 0.0f;
        float depth = 0.0f;
        modeComponents(mode, rx, ry, rz, sx, sy, depth);
        (void)depth;
        handles[i] = sf::Vector2f(center.x + sx * radius * 0.8f, center.y - sy * radius * 0.8f);
    }

    return GimbalOverlay{rect, center, radius, handles};
}

float distanceSquared(const sf::Vector2f &a, const sf::Vector2f &b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

GimbalAxis hitGimbalAxis(const GimbalOverlay &overlay, const sf::Vector2f &mouse)
{
    const float threshold2 = 11.0f * 11.0f;
    if (distanceSquared(mouse, overlay.handles[0]) <= threshold2) {
        return GimbalAxis::X;
    }
    if (distanceSquared(mouse, overlay.handles[1]) <= threshold2) {
        return GimbalAxis::Y;
    }
    if (distanceSquared(mouse, overlay.handles[2]) <= threshold2) {
        return GimbalAxis::Z;
    }
    return GimbalAxis::None;
}

void drawGimbal(sf::RenderWindow &window, const GimbalOverlay &overlay)
{
    sf::CircleShape shell(overlay.radius);
    shell.setPosition(sf::Vector2f(overlay.center.x - overlay.radius, overlay.center.y - overlay.radius));
    shell.setFillColor(sf::Color(18, 18, 26, 180));
    shell.setOutlineThickness(1.0f);
    shell.setOutlineColor(sf::Color(120, 120, 135));
    window.draw(shell);

#if SFML_VERSION_MAJOR >= 3
    sf::VertexArray lines(sf::PrimitiveType::Lines);
    sf::CircleShape handleShape(3.5f);
#else
    sf::VertexArray lines(sf::Lines);
    sf::CircleShape handleShape(3.5f);
#endif

    const std::array<sf::Color, 3> colors = {
        sf::Color(255, 95, 95),
        sf::Color(85, 255, 125),
        sf::Color(105, 150, 255)
    };

    for (std::size_t i = 0; i < overlay.handles.size(); ++i) {
        sf::Vertex v0;
        v0.position = overlay.center;
        v0.color = colors[i];
        lines.append(v0);

        sf::Vertex v1;
        v1.position = overlay.handles[i];
        v1.color = colors[i];
        lines.append(v1);

        handleShape.setFillColor(colors[i]);
        handleShape.setPosition(sf::Vector2f(overlay.handles[i].x - 3.5f, overlay.handles[i].y - 3.5f));
        window.draw(handleShape);
    }

    window.draw(lines);
}

void drawGraphBorder(sf::RenderWindow &window, const RectF &rect)
{
    sf::RectangleShape bg(sf::Vector2f(rect.width, rect.height));
    bg.setPosition(sf::Vector2f(rect.left, rect.top));
    bg.setFillColor(sf::Color(15, 15, 22));
    bg.setOutlineThickness(1.0f);
    bg.setOutlineColor(sf::Color(45, 45, 58));
    window.draw(bg);
}

bool loadFontFile(sf::Font &font, const std::string &path)
{
#if SFML_VERSION_MAJOR >= 3
    return font.openFromFile(path);
#else
    return font.loadFromFile(path);
#endif
}

std::optional<sf::Font> loadOverlayFont()
{
    const std::array<std::string, 4> candidates = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/Library/Fonts/Arial.ttf"
    };

    for (const std::string &path : candidates) {
        if (!std::filesystem::exists(path)) {
            continue;
        }
        sf::Font font;
        if (loadFontFile(font, path)) {
            return font;
        }
    }
    return std::nullopt;
}

void drawOverlayText(
    sf::RenderWindow &window,
    const sf::Font &font,
    const std::string &value,
    float x,
    float y,
    unsigned int size,
    const sf::Color &color
)
{
#if SFML_VERSION_MAJOR >= 3
    sf::Text text(font, value, size);
#else
    sf::Text text(value, font, size);
#endif
    text.setFillColor(color);
    text.setPosition(sf::Vector2f(x, y));
    window.draw(text);
}

float sampleMin(const std::vector<EnergyPoint> &history)
{
    float minValue = std::numeric_limits<float>::infinity();
    for (const EnergyPoint &e : history) {
        minValue = std::min(minValue, e.kinetic);
        minValue = std::min(minValue, e.potential);
        minValue = std::min(minValue, e.thermal);
        minValue = std::min(minValue, e.radiated);
        minValue = std::min(minValue, e.total);
    }
    return minValue;
}

float sampleMax(const std::vector<EnergyPoint> &history)
{
    float maxValue = -std::numeric_limits<float>::infinity();
    for (const EnergyPoint &e : history) {
        maxValue = std::max(maxValue, e.kinetic);
        maxValue = std::max(maxValue, e.potential);
        maxValue = std::max(maxValue, e.thermal);
        maxValue = std::max(maxValue, e.radiated);
        maxValue = std::max(maxValue, e.total);
    }
    return maxValue;
}

float graphY(float value, float minValue, float maxValue, const RectF &rect)
{
    if (maxValue <= minValue + 1e-9f) {
        return rect.top + rect.height * 0.5f;
    }
    const float t = (value - minValue) / (maxValue - minValue);
    return rect.top + rect.height * (1.0f - t);
}

std::string openSimulationFileDialog()
{
#if defined(_WIN32)
    char fileBuffer[MAX_PATH] = {};
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Simulation files (*.vtk;*.xyz;*.bin)\0*.vtk;*.xyz;*.bin\0All files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(fileBuffer);
    }
#endif
    return {};
}

std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string normalizeExportFormat(const std::string &raw)
{
    const std::string format = toLower(raw);
    if (format == "binary") {
        return "bin";
    }
    if (format == "vtkb" || format == "vtk-bin") {
        return "vtk_binary";
    }
    if (format == "vtk_ascii") {
        return "vtk";
    }
    return format;
}

std::string extensionForExportFormat(const std::string &rawFormat)
{
    const std::string format = normalizeExportFormat(rawFormat);
    if (format == "vtk" || format == "vtk_binary") {
        return ".vtk";
    }
    if (format == "xyz") {
        return ".xyz";
    }
    if (format == "bin") {
        return ".bin";
    }
    return {};
}

std::string inferExportFormatFromPath(const std::string &path)
{
    std::string ext = toLower(std::filesystem::path(path).extension().string());
    if (!ext.empty() && ext.front() == '.') {
        ext.erase(ext.begin());
    }
    if (ext == "vtk") {
        return "vtk";
    }
    if (ext == "xyz") {
        return "xyz";
    }
    if (ext == "bin" || ext == "binary" || ext == "nbin") {
        return "bin";
    }
    return {};
}

std::string formatFromSaveFilterIndex(unsigned long filterIndex)
{
    switch (filterIndex) {
        case 1:
            return "vtk";
        case 2:
            return "vtk_binary";
        case 3:
            return "xyz";
        case 4:
            return "bin";
        default:
            return {};
    }
}

struct ExportSelection {
    std::string path;
    std::string format;
};

std::string buildSuggestedExportPath(const std::string &defaultDirectory, const std::string &defaultFormat, std::uint64_t step)
{
    const std::string normalizedFormat = normalizeExportFormat(defaultFormat.empty() ? std::string("vtk") : defaultFormat);
    const std::string extension = extensionForExportFormat(normalizedFormat);

    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &nowTime);
#else
    localtime_r(&nowTime, &tm);
#endif

    std::ostringstream fileName;
    fileName << "sim_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_s" << step << extension;
    const std::filesystem::path baseDir = defaultDirectory.empty() ? std::filesystem::path("exports") : std::filesystem::path(defaultDirectory);
    return (baseDir / fileName.str()).string();
}

ExportSelection openExportFileDialog(const std::string &defaultDirectory, const std::string &defaultFormat, std::uint64_t step)
{
#if defined(_WIN32)
    char fileBuffer[MAX_PATH] = {};
    const std::string defaultPathStr = buildSuggestedExportPath(defaultDirectory, defaultFormat, step);
    std::strncpy(fileBuffer, defaultPathStr.c_str(), MAX_PATH - 1);
    fileBuffer[MAX_PATH - 1] = '\0';

    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter =
        "VTK ASCII (*.vtk)\0*.vtk\0"
        "VTK BINARY (*.vtk)\0*.vtk\0"
        "XYZ (*.xyz)\0*.xyz\0"
        "Native binary (*.bin)\0*.bin\0"
        "All files (*.*)\0*.*\0";
    unsigned long defaultFilter = 1;
    const std::string normalizedDefault = normalizeExportFormat(defaultFormat);
    if (normalizedDefault == "vtk_binary") {
        defaultFilter = 2;
    } else if (normalizedDefault == "xyz") {
        defaultFilter = 3;
    } else if (normalizedDefault == "bin") {
        defaultFilter = 4;
    }
    ofn.nFilterIndex = defaultFilter;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn) == TRUE) {
        std::string selectedPath(fileBuffer);
        std::string selectedFormat = formatFromSaveFilterIndex(ofn.nFilterIndex);
        if (selectedFormat.empty()) {
            selectedFormat = inferExportFormatFromPath(selectedPath);
        }
        if (selectedFormat.empty()) {
            selectedFormat = normalizedDefault.empty() ? std::string("vtk") : normalizedDefault;
        }

        std::filesystem::path outPath(selectedPath);
        if (outPath.extension().empty()) {
            const std::string ext = extensionForExportFormat(selectedFormat);
            if (!ext.empty()) {
                outPath += ext;
                selectedPath = outPath.string();
            }
        }
        return ExportSelection{selectedPath, selectedFormat};
    }
#endif
    return ExportSelection{};
}

void appendEnergySample(std::vector<EnergyPoint> &history, const SimulationStats &stats)
{
    history.push_back(EnergyPoint{
        stats.kineticEnergy,
        stats.potentialEnergy,
        stats.thermalEnergy,
        stats.radiatedEnergy,
        stats.totalEnergy,
        stats.energyDriftPct
    });
    if (history.size() > kEnergyHistoryMax) {
        history.erase(history.begin(), history.begin() + static_cast<long long>(history.size() - kEnergyHistoryMax));
    }
}

void drawEnergyGraph(
    sf::RenderWindow &window,
    const RectF &graphRect,
    const std::vector<EnergyPoint> &history,
    const sf::Font *overlayFont
)
{
    drawGraphBorder(window, graphRect);
    if (history.size() < 2) {
        return;
    }

    const float split = graphRect.height * 0.74f;
    const RectF energyRect{graphRect.left + 6.0f, graphRect.top + 6.0f, graphRect.width - 12.0f, split - 10.0f};
    const RectF driftRect{graphRect.left + 6.0f, graphRect.top + split + 2.0f, graphRect.width - 12.0f, graphRect.height - split - 8.0f};

    sf::RectangleShape separator(sf::Vector2f(energyRect.width, 1.0f));
    separator.setPosition(sf::Vector2f(energyRect.left, graphRect.top + split));
    separator.setFillColor(sf::Color(40, 40, 52));
    window.draw(separator);

    const float minEnergy = sampleMin(history);
    const float maxEnergy = sampleMax(history);

    float maxAbsDrift = 0.01f;
    for (const EnergyPoint &e : history) {
        maxAbsDrift = std::max(maxAbsDrift, std::fabs(e.drift));
    }

#if SFML_VERSION_MAJOR >= 3
    sf::VertexArray kineticLine(sf::PrimitiveType::LineStrip);
    sf::VertexArray potentialLine(sf::PrimitiveType::LineStrip);
    sf::VertexArray thermalLine(sf::PrimitiveType::LineStrip);
    sf::VertexArray radiatedLine(sf::PrimitiveType::LineStrip);
    sf::VertexArray totalLine(sf::PrimitiveType::LineStrip);
    sf::VertexArray driftLine(sf::PrimitiveType::LineStrip);
#else
    sf::VertexArray kineticLine(sf::LineStrip);
    sf::VertexArray potentialLine(sf::LineStrip);
    sf::VertexArray thermalLine(sf::LineStrip);
    sf::VertexArray radiatedLine(sf::LineStrip);
    sf::VertexArray totalLine(sf::LineStrip);
    sf::VertexArray driftLine(sf::LineStrip);
#endif

    const float xStep = energyRect.width / static_cast<float>(history.size() - 1);
    const float xStepDrift = driftRect.width / static_cast<float>(history.size() - 1);

    for (std::size_t i = 0; i < history.size(); ++i) {
        const float x = energyRect.left + xStep * static_cast<float>(i);
        const float yK = graphY(history[i].kinetic, minEnergy, maxEnergy, energyRect);
        const float yP = graphY(history[i].potential, minEnergy, maxEnergy, energyRect);
        const float yH = graphY(history[i].thermal, minEnergy, maxEnergy, energyRect);
        const float yR = graphY(history[i].radiated, minEnergy, maxEnergy, energyRect);
        const float yT = graphY(history[i].total, minEnergy, maxEnergy, energyRect);

        sf::Vertex vk;
        vk.position = sf::Vector2f(x, yK);
        vk.color = sf::Color(92, 255, 140);
        kineticLine.append(vk);

        sf::Vertex vp;
        vp.position = sf::Vector2f(x, yP);
        vp.color = sf::Color(255, 120, 108);
        potentialLine.append(vp);

        sf::Vertex vt;
        vt.position = sf::Vector2f(x, yT);
        vt.color = sf::Color(120, 200, 255);
        totalLine.append(vt);

        sf::Vertex vh;
        vh.position = sf::Vector2f(x, yH);
        vh.color = sf::Color(255, 170, 90);
        thermalLine.append(vh);

        sf::Vertex vr;
        vr.position = sf::Vector2f(x, yR);
        vr.color = sf::Color(180, 120, 255);
        radiatedLine.append(vr);

        const float xd = driftRect.left + xStepDrift * static_cast<float>(i);
        const float driftNorm = (history[i].drift / maxAbsDrift) * 0.5f + 0.5f;
        const float yd = driftRect.top + driftRect.height * (1.0f - driftNorm);
        sf::Vertex vd;
        vd.position = sf::Vector2f(xd, yd);
        vd.color = sf::Color(255, 230, 120);
        driftLine.append(vd);
    }

    const float zeroY = driftRect.top + driftRect.height * 0.5f;
#if SFML_VERSION_MAJOR >= 3
    sf::VertexArray zeroLine(sf::PrimitiveType::Lines, 2);
#else
    sf::VertexArray zeroLine(sf::Lines, 2);
#endif
    zeroLine[0].position = sf::Vector2f(driftRect.left, zeroY);
    zeroLine[0].color = sf::Color(70, 70, 84);
    zeroLine[1].position = sf::Vector2f(driftRect.left + driftRect.width, zeroY);
    zeroLine[1].color = sf::Color(70, 70, 84);

    window.draw(kineticLine);
    window.draw(potentialLine);
    window.draw(thermalLine);
    window.draw(radiatedLine);
    window.draw(totalLine);
    window.draw(zeroLine);
    window.draw(driftLine);

    sf::RectangleShape legendBar(sf::Vector2f(16.0f, 2.0f));
    legendBar.setPosition(sf::Vector2f(energyRect.left + 6.0f, energyRect.top + 6.0f));
    legendBar.setFillColor(sf::Color(92, 255, 140));
    window.draw(legendBar);
    legendBar.setPosition(sf::Vector2f(energyRect.left + 28.0f, energyRect.top + 6.0f));
    legendBar.setFillColor(sf::Color(255, 120, 108));
    window.draw(legendBar);
    legendBar.setPosition(sf::Vector2f(energyRect.left + 50.0f, energyRect.top + 6.0f));
    legendBar.setFillColor(sf::Color(255, 170, 90));
    window.draw(legendBar);
    legendBar.setPosition(sf::Vector2f(energyRect.left + 72.0f, energyRect.top + 6.0f));
    legendBar.setFillColor(sf::Color(180, 120, 255));
    window.draw(legendBar);
    legendBar.setPosition(sf::Vector2f(energyRect.left + 94.0f, energyRect.top + 6.0f));
    legendBar.setFillColor(sf::Color(120, 200, 255));
    window.draw(legendBar);
    legendBar.setPosition(sf::Vector2f(energyRect.left + 116.0f, energyRect.top + 6.0f));
    legendBar.setFillColor(sf::Color(255, 230, 120));
    window.draw(legendBar);

    if (overlayFont != nullptr) {
        drawOverlayText(window, *overlayFont, "Energy", energyRect.left + 6.0f, energyRect.top - 2.0f, 12u, sf::Color(195, 195, 208));
        drawOverlayText(window, *overlayFont, "dE %", driftRect.left + 6.0f, driftRect.top - 2.0f, 12u, sf::Color(195, 195, 208));
        drawOverlayText(window, *overlayFont, "samples ->", driftRect.left + driftRect.width - 78.0f, driftRect.top + driftRect.height - 16.0f, 10u, sf::Color(150, 150, 162));

        const float legendY = energyRect.top + 0.0f;
        drawOverlayText(window, *overlayFont, "Ekin", energyRect.left + 2.0f, legendY + 8.0f, 10u, sf::Color(190, 190, 204));
        drawOverlayText(window, *overlayFont, "Epot", energyRect.left + 24.0f, legendY + 8.0f, 10u, sf::Color(190, 190, 204));
        drawOverlayText(window, *overlayFont, "Eth", energyRect.left + 48.0f, legendY + 8.0f, 10u, sf::Color(190, 190, 204));
        drawOverlayText(window, *overlayFont, "Erad", energyRect.left + 68.0f, legendY + 8.0f, 10u, sf::Color(190, 190, 204));
        drawOverlayText(window, *overlayFont, "Etot", energyRect.left + 92.0f, legendY + 8.0f, 10u, sf::Color(190, 190, 204));
        drawOverlayText(window, *overlayFont, "dE%", energyRect.left + 116.0f, legendY + 8.0f, 10u, sf::Color(190, 190, 204));
    }
}

#if SFML_VERSION_MAJOR >= 3
constexpr sf::Keyboard::Key kKeyAdd = sf::Keyboard::Key::Add;
constexpr sf::Keyboard::Key kKeySubtract = sf::Keyboard::Key::Subtract;
constexpr sf::Keyboard::Key kKeyUp = sf::Keyboard::Key::Up;
constexpr sf::Keyboard::Key kKeyDown = sf::Keyboard::Key::Down;
constexpr sf::Keyboard::Key kKeyP = sf::Keyboard::Key::P;
constexpr sf::Keyboard::Key kKeyM = sf::Keyboard::Key::M;
constexpr sf::Keyboard::Key kKeySpace = sf::Keyboard::Key::Space;
constexpr sf::Keyboard::Key kKeyEnter = sf::Keyboard::Key::Enter;
constexpr sf::Keyboard::Key kKeyR = sf::Keyboard::Key::R;
constexpr sf::Keyboard::Key kKeyNum1 = sf::Keyboard::Key::Num1;
constexpr sf::Keyboard::Key kKeyNum2 = sf::Keyboard::Key::Num2;
constexpr sf::Keyboard::Key kKeyNum3 = sf::Keyboard::Key::Num3;
constexpr sf::Keyboard::Key kKeyF = sf::Keyboard::Key::F;
constexpr sf::Keyboard::Key kKeyL = sf::Keyboard::Key::L;
constexpr sf::Keyboard::Key kKeyE = sf::Keyboard::Key::E;
constexpr sf::Keyboard::Key kKeyO = sf::Keyboard::Key::O;
constexpr sf::Keyboard::Key kKeyA = sf::Keyboard::Key::A;
constexpr sf::Keyboard::Key kKeyD = sf::Keyboard::Key::D;
constexpr sf::Keyboard::Key kKeyW = sf::Keyboard::Key::W;
constexpr sf::Keyboard::Key kKeyS = sf::Keyboard::Key::S;
constexpr sf::Keyboard::Key kKeyQ = sf::Keyboard::Key::Q;
constexpr sf::Keyboard::Key kKeyZ = sf::Keyboard::Key::Z;
constexpr sf::Keyboard::Key kKeyT = sf::Keyboard::Key::T;
constexpr sf::Keyboard::Key kKeyG = sf::Keyboard::Key::G;
constexpr sf::Keyboard::Key kKeyY = sf::Keyboard::Key::Y;
constexpr sf::Keyboard::Key kKeyH = sf::Keyboard::Key::H;
constexpr sf::Keyboard::Key kKeyI = sf::Keyboard::Key::I;
constexpr sf::Keyboard::Key kKeyU = sf::Keyboard::Key::U;
#else
constexpr sf::Keyboard::Key kKeyAdd = sf::Keyboard::Add;
constexpr sf::Keyboard::Key kKeySubtract = sf::Keyboard::Subtract;
constexpr sf::Keyboard::Key kKeyUp = sf::Keyboard::Up;
constexpr sf::Keyboard::Key kKeyDown = sf::Keyboard::Down;
constexpr sf::Keyboard::Key kKeyP = sf::Keyboard::P;
constexpr sf::Keyboard::Key kKeyM = sf::Keyboard::M;
constexpr sf::Keyboard::Key kKeySpace = sf::Keyboard::Space;
constexpr sf::Keyboard::Key kKeyEnter = sf::Keyboard::Enter;
constexpr sf::Keyboard::Key kKeyR = sf::Keyboard::R;
constexpr sf::Keyboard::Key kKeyNum1 = sf::Keyboard::Num1;
constexpr sf::Keyboard::Key kKeyNum2 = sf::Keyboard::Num2;
constexpr sf::Keyboard::Key kKeyNum3 = sf::Keyboard::Num3;
constexpr sf::Keyboard::Key kKeyF = sf::Keyboard::F;
constexpr sf::Keyboard::Key kKeyL = sf::Keyboard::L;
constexpr sf::Keyboard::Key kKeyE = sf::Keyboard::E;
constexpr sf::Keyboard::Key kKeyO = sf::Keyboard::O;
constexpr sf::Keyboard::Key kKeyA = sf::Keyboard::A;
constexpr sf::Keyboard::Key kKeyD = sf::Keyboard::D;
constexpr sf::Keyboard::Key kKeyW = sf::Keyboard::W;
constexpr sf::Keyboard::Key kKeyS = sf::Keyboard::S;
constexpr sf::Keyboard::Key kKeyQ = sf::Keyboard::Q;
constexpr sf::Keyboard::Key kKeyZ = sf::Keyboard::Z;
constexpr sf::Keyboard::Key kKeyT = sf::Keyboard::T;
constexpr sf::Keyboard::Key kKeyG = sf::Keyboard::G;
constexpr sf::Keyboard::Key kKeyY = sf::Keyboard::Y;
constexpr sf::Keyboard::Key kKeyH = sf::Keyboard::H;
constexpr sf::Keyboard::Key kKeyI = sf::Keyboard::I;
constexpr sf::Keyboard::Key kKeyU = sf::Keyboard::U;
#endif

template <typename KeyType>
void handleKeyPress(KeyType code, SimulationBackend &backend, UiControls &ui, CameraState &mainCamera)
{
    if (code == kKeyAdd) {
        ui.luminosity += 5.0;
    }
    if (code == kKeySubtract) {
        ui.luminosity -= 5.0;
    }
    if (code == kKeyUp) {
        backend.scaleDt(1.02f);
    }
    if (code == kKeyDown) {
        backend.scaleDt(0.98f);
    }
    if (code == kKeyP) {
        ui.zoom *= 1.1f;
    }
    if (code == kKeyM) {
        ui.zoom *= 0.9f;
    }
    if (code == kKeySpace) {
        backend.togglePaused();
    }
    if (code == kKeyEnter) {
        backend.stepOnce();
    }
    if (code == kKeyR) {
        backend.requestReset();
    }
    if (code == kKeyNum1) {
        backend.setSolverMode("pairwise_cuda");
    }
    if (code == kKeyNum2) {
        backend.setSolverMode("octree_gpu");
    }
    if (code == kKeyNum3) {
        backend.setSolverMode("octree_cpu");
    }
    if (code == kKeyF) {
        ui.sphEnabled = !ui.sphEnabled;
        backend.setSphEnabled(ui.sphEnabled);
        std::cout << "[sfml] sph=" << (ui.sphEnabled ? "on" : "off") << "\n";
    }
    if (code == kKeyL) {
        const std::string selected = openSimulationFileDialog();
        if (!selected.empty()) {
            backend.setInitialStateFile(selected, "auto");
            backend.requestReset();
            std::cout << "[sfml] input_file=" << selected << "\n";
        }
    }
    if (code == kKeyE) {
        const SimulationStats stats = backend.getStats();
        const ExportSelection selection = openExportFileDialog(ui.exportDirectory, ui.exportFormat, stats.steps);
        if (!selection.path.empty()) {
            backend.requestExportSnapshot(selection.path, selection.format);
            ui.exportFormat = selection.format;
            const std::filesystem::path exportedPath(selection.path);
            if (exportedPath.has_parent_path()) {
                ui.exportDirectory = exportedPath.parent_path().string();
            }
        }
    }
    if (code == kKeyO) {
        ui.perspective3d = !ui.perspective3d;
    }

    if (code == kKeyA) {
        mainCamera.yaw -= 0.05f;
    }
    if (code == kKeyD) {
        mainCamera.yaw += 0.05f;
    }
    if (code == kKeyW) {
        mainCamera.pitch += 0.05f;
    }
    if (code == kKeyS) {
        mainCamera.pitch -= 0.05f;
    }
    if (code == kKeyQ) {
        mainCamera.roll -= 0.05f;
    }
    if (code == kKeyZ) {
        mainCamera.roll += 0.05f;
    }
    if (code == kKeyI) {
        backend.setIntegratorMode("rk4");
    }
    if (code == kKeyU) {
        backend.setIntegratorMode("euler");
    }

    bool octreeChanged = false;
    if (code == kKeyT) {
        ui.octreeTheta *= 1.05f;
        octreeChanged = true;
    }
    if (code == kKeyG) {
        ui.octreeTheta *= 0.95f;
        octreeChanged = true;
    }
    if (code == kKeyY) {
        ui.octreeSoftening *= 1.05f;
        octreeChanged = true;
    }
    if (code == kKeyH) {
        ui.octreeSoftening *= 0.95f;
        octreeChanged = true;
    }
    if (octreeChanged) {
        ui.octreeTheta = std::clamp(ui.octreeTheta, 0.05f, 4.0f);
        ui.octreeSoftening = std::clamp(ui.octreeSoftening, 1e-4f, 5.0f);
        backend.setOctreeParameters(ui.octreeTheta, ui.octreeSoftening);
    }

    ui.luminosity = std::clamp(ui.luminosity, 0.0, 255.0);
    ui.zoom = std::clamp(ui.zoom, 0.1f, 500.0f);
}

std::string buildWindowTitle(const SimulationStats &stats, const UiControls &ui, float uiFps)
{
    std::ostringstream oss;
    oss << "N-Body | " << (stats.paused ? "PAUSED" : "RUNNING")
        << " | solver=" << stats.solverName
        << " | dt=" << std::fixed << std::setprecision(4) << stats.dt
        << " | backend=" << std::setprecision(1) << stats.backendFps << " step/s"
        << " | ui=" << std::setprecision(1) << uiFps << " fps"
        << " | steps=" << stats.steps
        << " | E=" << std::setprecision(3) << stats.totalEnergy
        << " Eth=" << stats.thermalEnergy
        << " Erad=" << stats.radiatedEnergy
        << " | dE=" << std::setprecision(3) << stats.energyDriftPct << "%"
        << (stats.energyEstimated ? " (est)" : "")
        << " | theta=" << std::setprecision(2) << ui.octreeTheta
        << " soft=" << ui.octreeSoftening
        << " | sph=" << (ui.sphEnabled ? "on" : "off")
        << " | view3d=" << (ui.perspective3d ? "perspective" : "iso");
    return oss.str();
}

std::uint32_t getRequestedBackendParticleCount(const SimulationConfig &config)
{
    const std::uint32_t baseCount = std::max<std::uint32_t>(2u, config.particleCount);

    std::uint32_t requested = baseCount;
    if (const char *legacy = std::getenv("GRAVITY_FRONTEND_PARTICLES")) {
        (void)legacy;
        static bool warnedLegacy = false;
        if (!warnedLegacy) {
            std::cout << "[sfml] ignoring deprecated GRAVITY_FRONTEND_PARTICLES; "
                         "use GRAVITY_BACKEND_PARTICLES for backend and "
                         "GRAVITY_FRONTEND_DRAW_CAP for frontend draw cap\n";
            warnedLegacy = true;
        }
    }
    if (const char *envValue = std::getenv("GRAVITY_BACKEND_PARTICLES")) {
        char *end = nullptr;
        const long parsed = std::strtol(envValue, &end, 10);
        if (end != envValue && parsed > 1) {
            requested = static_cast<std::uint32_t>(parsed);
        }
    }

    return requested;
}

std::uint32_t getFrontendDrawCap(const SimulationConfig &config)
{
    const std::uint32_t cap = std::max<std::uint32_t>(2u, config.frontendParticleCap);
    if (const char *envValue = std::getenv("GRAVITY_FRONTEND_DRAW_CAP")) {
        char *end = nullptr;
        const long parsed = std::strtol(envValue, &end, 10);
        if (end != envValue && parsed > 1) {
            return static_cast<std::uint32_t>(parsed);
        }
    }
    return cap;
}

void printControls()
{
    std::cout << "[sfml] controls: SPACE pause/resume | ENTER step | R reset | UP/DOWN dt | P/M zoom | +/- luminosity\n";
    std::cout << "[sfml] controls: 1 pairwise | 2 octree_gpu | 3 octree_cpu | F sph on/off | T/G theta +/- | Y/H softening +/-\n";
    std::cout << "[sfml] controls: W/A/S/D + Q/Z = yaw/pitch/roll camera 3D | O toggle iso/perspective | E export (save dialog) | L load file\n";
    std::cout << "[sfml] controls: I = integrator rk4 | U = integrator euler\n";
    std::cout << "[sfml] mouse: drag handles X/Y/Z in top-right gimbal of each panel\n";
}

bool pointInside(const RectF &rect, const sf::Vector2f &p)
{
    return p.x >= rect.left && p.x <= rect.left + rect.width && p.y >= rect.top && p.y <= rect.top + rect.height;
}

}

int main(int argc, char **argv)
{
    RuntimeArgs runtime;
    runtime.configPath = findConfigPathArg(argc, argv, "simulation.ini");
    SimulationConfig config = SimulationConfig::loadOrCreate(runtime.configPath);
    applyArgsToConfig(argc, argv, config, runtime, std::cerr);
    if (runtime.showHelp) {
        printUsage(std::cout, (argc > 0 && argv[0]) ? argv[0] : "myAppFrontend.exe", false);
        return 0;
    }
    if (runtime.saveConfig) {
        config.save(runtime.configPath);
    }

#if SFML_VERSION_MAJOR >= 3
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1400u, 920u)), "N-Body Simulation");
#else
    sf::RenderWindow window(sf::VideoMode(1400, 920), "N-Body Simulation");
#endif

    window.setFramerateLimit(std::max<std::uint32_t>(10u, config.uiFpsLimit));
    const std::optional<sf::Font> overlayFont = loadOverlayFont();
    if (!overlayFont.has_value()) {
        std::cout << "[sfml] no overlay font found (graph text labels disabled)\n";
    }

    SimulationBackend backend(getRequestedBackendParticleCount(config), config.dt);
    backend.setSolverMode(config.solver);
    backend.setIntegratorMode(config.integrator);
    backend.setOctreeParameters(config.octreeTheta, config.octreeSoftening);
    backend.setSphEnabled(config.sphEnabled);
    backend.setSphParameters(
        config.sphSmoothingLength,
        config.sphRestDensity,
        config.sphGasConstant,
        config.sphViscosity
    );
    backend.setEnergyMeasurementConfig(config.energyMeasureEverySteps, config.energySampleLimit);
    backend.setExportDefaults(config.exportDirectory, config.exportFormat);
    backend.setInitialStateFile(config.inputFile, config.inputFormat);
    backend.setInitialStateConfig(buildInitialStateConfig(config));
    backend.start();

    UiControls ui{
        std::clamp<double>(config.defaultLuminosity, 0.0, 255.0),
        std::max(0.1f, config.defaultZoom),
        true,
        config.sphEnabled,
        std::max(0.05f, config.octreeTheta),
        std::max(1e-4f, config.octreeSoftening),
        config.exportFormat.empty() ? std::string("vtk") : config.exportFormat,
        config.exportDirectory.empty() ? std::string("exports") : config.exportDirectory
    };

    std::array<CameraState, 4> cameras = {
        CameraState{0.0f, 0.0f, 0.0f},
        CameraState{0.0f, 0.0f, 0.0f},
        CameraState{0.0f, 0.0f, 0.0f},
        CameraState{0.0f, 0.0f, 0.0f}
    };

    printControls();

    std::vector<RenderParticle> drawSnapshot;
    std::vector<std::size_t> drawIndices;
    std::vector<EnergyPoint> energyHistory;
    std::vector<sf::Color> drawColors;
    std::uint32_t frontendDrawCap = getFrontendDrawCap(config);
    std::uint64_t lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
    DragState drag{false, -1, GimbalAxis::None, sf::Vector2f(0.0f, 0.0f)};
    const std::array<sf::Color, kTempBins> temperatureLut = buildTemperatureLut();
    std::array<std::uint8_t, kPressureBins> alphaLut = buildAlphaLut(ui.luminosity);
    int lastLuminosityQuantized = static_cast<int>(std::clamp(ui.luminosity, 0.0, 255.0));
    float adaptiveTemperatureScale = 2.0f;
    float adaptivePressureScale = 2.0f;

    auto lastTitleUpdate = std::chrono::steady_clock::now();
    auto lastConsoleTrace = std::chrono::steady_clock::now();
    auto lastUiFrameAt = std::chrono::steady_clock::now();
    float uiFps = 0.0f;
    auto reloadConfigAndReset = [&]() {
        config = SimulationConfig::loadOrCreate(runtime.configPath);
        backend.setParticleCount(getRequestedBackendParticleCount(config));
        backend.setDt(config.dt);
        backend.setSolverMode(config.solver);
        backend.setIntegratorMode(config.integrator);
        backend.setOctreeParameters(config.octreeTheta, config.octreeSoftening);
        backend.setSphEnabled(config.sphEnabled);
        backend.setSphParameters(
            config.sphSmoothingLength,
            config.sphRestDensity,
            config.sphGasConstant,
            config.sphViscosity
        );
        backend.setEnergyMeasurementConfig(config.energyMeasureEverySteps, config.energySampleLimit);
        backend.setExportDefaults(config.exportDirectory, config.exportFormat);
        backend.setInitialStateFile(config.inputFile, config.inputFormat);
        backend.setInitialStateConfig(buildInitialStateConfig(config));

        ui.luminosity = std::clamp<double>(config.defaultLuminosity, 0.0, 255.0);
        ui.zoom = std::max(0.1f, config.defaultZoom);
        ui.sphEnabled = config.sphEnabled;
        ui.octreeTheta = std::max(0.05f, config.octreeTheta);
        ui.octreeSoftening = std::max(1e-4f, config.octreeSoftening);
        ui.exportFormat = config.exportFormat.empty() ? std::string("vtk") : config.exportFormat;
        ui.exportDirectory = config.exportDirectory.empty() ? std::string("exports") : config.exportDirectory;
        frontendDrawCap = getFrontendDrawCap(config);

        drawSnapshot.clear();
        energyHistory.clear();
        lastEnergyStep = std::numeric_limits<std::uint64_t>::max();
        alphaLut = buildAlphaLut(ui.luminosity);
        lastLuminosityQuantized = static_cast<int>(std::clamp(ui.luminosity, 0.0, 255.0));
        adaptiveTemperatureScale = 2.0f;
        adaptivePressureScale = 2.0f;

        backend.requestReset();
        std::cout << "[sfml] config reloaded from " << runtime.configPath << "\n";
    };

    while (window.isOpen()) {
        const auto uiNow = std::chrono::steady_clock::now();
        const std::chrono::duration<float> uiDt = uiNow - lastUiFrameAt;
        if (uiDt.count() > 1e-6f) {
            const float inst = 1.0f / uiDt.count();
            uiFps = (uiFps <= 0.0f) ? inst : (0.9f * uiFps + 0.1f * inst);
        }
        lastUiFrameAt = uiNow;

        const Layout layout = computeLayout(window.getSize(), ui.perspective3d);
        std::array<GimbalOverlay, 4> gimbals = {
            computeGimbal(layout.panels[0], layout.panels[0].mode, cameras[0]),
            computeGimbal(layout.panels[1], layout.panels[1].mode, cameras[1]),
            computeGimbal(layout.panels[2], layout.panels[2].mode, cameras[2]),
            computeGimbal(layout.panels[3], layout.panels[3].mode, cameras[3])
        };

#if SFML_VERSION_MAJOR >= 3
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto *key = event->getIf<sf::Event::KeyPressed>()) {
                handleKeyPress(key->code, backend, ui, cameras[3]);
                if (key->code == kKeyR) {
                    reloadConfigAndReset();
                }
            }
            if (const auto *pressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (pressed->button == sf::Mouse::Button::Left) {
                    const sf::Vector2f mousePos(static_cast<float>(pressed->position.x), static_cast<float>(pressed->position.y));
                    for (int i = 0; i < 4; ++i) {
                        if (!pointInside(gimbals[i].rect, mousePos)) {
                            continue;
                        }
                        const GimbalAxis axis = hitGimbalAxis(gimbals[i], mousePos);
                        if (axis != GimbalAxis::None) {
                            drag.active = true;
                            drag.panelIndex = i;
                            drag.axis = axis;
                            drag.lastPos = mousePos;
                            break;
                        }
                    }
                }
            }
            if (event->is<sf::Event::MouseButtonReleased>()) {
                drag.active = false;
                drag.panelIndex = -1;
                drag.axis = GimbalAxis::None;
            }
            if (const auto *moved = event->getIf<sf::Event::MouseMoved>()) {
                if (drag.active && drag.panelIndex >= 0 && drag.panelIndex < 4) {
                    const sf::Vector2f pos(static_cast<float>(moved->position.x), static_cast<float>(moved->position.y));
                    const float dx = pos.x - drag.lastPos.x;
                    const float dy = pos.y - drag.lastPos.y;
                    const float delta = (dx - dy) * 0.01f;
                    CameraState &camera = cameras[drag.panelIndex];
                    if (drag.axis == GimbalAxis::X) {
                        camera.pitch += delta;
                    } else if (drag.axis == GimbalAxis::Y) {
                        camera.yaw += delta;
                    } else if (drag.axis == GimbalAxis::Z) {
                        camera.roll += delta;
                    }
                    drag.lastPos = pos;
                }
            }
        }
#else
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed) {
                handleKeyPress(event.key.code, backend, ui, cameras[3]);
                if (event.key.code == kKeyR) {
                    reloadConfigAndReset();
                }
            }
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                const sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
                for (int i = 0; i < 4; ++i) {
                    if (!pointInside(gimbals[i].rect, mousePos)) {
                        continue;
                    }
                    const GimbalAxis axis = hitGimbalAxis(gimbals[i], mousePos);
                    if (axis != GimbalAxis::None) {
                        drag.active = true;
                        drag.panelIndex = i;
                        drag.axis = axis;
                        drag.lastPos = mousePos;
                        break;
                    }
                }
            }
            if (event.type == sf::Event::MouseButtonReleased) {
                drag.active = false;
                drag.panelIndex = -1;
                drag.axis = GimbalAxis::None;
            }
            if (event.type == sf::Event::MouseMoved) {
                if (drag.active && drag.panelIndex >= 0 && drag.panelIndex < 4) {
                    const sf::Vector2f pos(static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y));
                    const float dx = pos.x - drag.lastPos.x;
                    const float dy = pos.y - drag.lastPos.y;
                    const float delta = (dx - dy) * 0.01f;
                    CameraState &camera = cameras[drag.panelIndex];
                    if (drag.axis == GimbalAxis::X) {
                        camera.pitch += delta;
                    } else if (drag.axis == GimbalAxis::Y) {
                        camera.yaw += delta;
                    } else if (drag.axis == GimbalAxis::Z) {
                        camera.roll += delta;
                    }
                    drag.lastPos = pos;
                }
            }
        }
#endif

        backend.tryConsumeSnapshot(drawSnapshot);
        drawIndices.clear();
        const std::size_t drawCap = std::max<std::size_t>(2u, frontendDrawCap);
        if (drawSnapshot.size() <= drawCap) {
            drawIndices.resize(drawSnapshot.size());
            std::iota(drawIndices.begin(), drawIndices.end(), static_cast<std::size_t>(0));
        } else {
            drawIndices.reserve(drawCap);
            const std::size_t stride = (drawSnapshot.size() + drawCap - 1u) / drawCap;
            for (std::size_t i = 0; i < drawSnapshot.size() && drawIndices.size() < drawCap; i += stride) {
                drawIndices.push_back(i);
            }
        }
        const SimulationStats stats = backend.getStats();
        const int luminosityQuantized = static_cast<int>(std::clamp(ui.luminosity, 0.0, 255.0));
        if (luminosityQuantized != lastLuminosityQuantized) {
            alphaLut = buildAlphaLut(ui.luminosity);
            lastLuminosityQuantized = luminosityQuantized;
        }

        float observedTempMax = kTemperatureScaleFloor;
        float observedPressureMax = kPressureScaleFloor;
        for (std::size_t sampledIndex : drawIndices) {
            const RenderParticle &particle = drawSnapshot[sampledIndex];
            observedTempMax = std::max(observedTempMax, std::max(0.0f, particle.temperature));
            observedPressureMax = std::max(observedPressureMax, std::max(0.0f, particle.pressureNorm));
        }
        adaptiveTemperatureScale = updateAdaptiveScale(adaptiveTemperatureScale, observedTempMax, kTemperatureScaleFloor, 0.32f, 0.04f);
        adaptivePressureScale = updateAdaptiveScale(adaptivePressureScale, observedPressureMax, kPressureScaleFloor, 0.32f, 0.04f);

        drawColors.resize(drawIndices.size());
        for (std::size_t i = 0; i < drawIndices.size(); ++i) {
            const RenderParticle &particle = drawSnapshot[drawIndices[i]];
            if (particle.mass > 100.0f) {
                drawColors[i] = sf::Color::Red;
            } else {
                drawColors[i] = colorRampFast(
                    particle.temperature,
                    particle.pressureNorm,
                    adaptiveTemperatureScale,
                    adaptivePressureScale,
                    temperatureLut,
                    alphaLut
                );
            }
        }

        if (stats.steps != lastEnergyStep) {
            appendEnergySample(energyHistory, stats);
            lastEnergyStep = stats.steps;
        }

        window.clear(sf::Color(12, 12, 18));

        for (int panelIndex = 0; panelIndex < 4; ++panelIndex) {
            const ViewPanel &panel = layout.panels[panelIndex];
            const CameraState &camera = cameras[panelIndex];

#if SFML_VERSION_MAJOR >= 3
            sf::VertexArray points(sf::PrimitiveType::Points);
#else
            sf::VertexArray points(sf::Points);
#endif

            const float centerX = panel.x + panel.width * 0.5f;
            const float centerY = panel.y + panel.height * 0.5f;

            for (std::size_t i = 0; i < drawIndices.size(); ++i) {
                const RenderParticle &particle = drawSnapshot[drawIndices[i]];
                const ProjectedPoint pp = projectParticle(particle, panel.mode, camera);
                if (!pp.valid) {
                    continue;
                }

                const float sx = centerX + pp.x * ui.zoom;
                const float sy = centerY - pp.y * ui.zoom;
                if (sx < panel.x || sx >= (panel.x + panel.width) || sy < panel.y || sy >= (panel.y + panel.height)) {
                    continue;
                }

                sf::Vertex v;
                v.position = sf::Vector2f(sx, sy);
                v.color = drawColors[i];
                points.append(v);
            }

            window.draw(points);

            sf::RectangleShape border(sf::Vector2f(std::max(0.0f, panel.width - 1.0f), std::max(0.0f, panel.height - 1.0f)));
            border.setPosition(sf::Vector2f(panel.x, panel.y));
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineThickness(1.0f);
            border.setOutlineColor(sf::Color(45, 45, 58));
            window.draw(border);

            drawGimbal(window, gimbals[panelIndex]);
        }

        drawEnergyGraph(window, layout.graphRect, energyHistory, overlayFont.has_value() ? &overlayFont.value() : nullptr);

        window.display();

        const auto now = std::chrono::steady_clock::now();
        if (now - lastConsoleTrace >= std::chrono::seconds(1)) {
            std::cout << "[sfml] step=" << stats.steps
                      << " solver=" << stats.solverName
                      << " step_s=" << stats.backendFps
                      << " ui_fps=" << uiFps
                      << " snapshot=" << drawSnapshot.size()
                      << " draw=" << drawIndices.size()
                      << " draw_cap=" << frontendDrawCap
                      << " energy=" << stats.totalEnergy
                      << " thermal=" << stats.thermalEnergy
                      << " radiated=" << stats.radiatedEnergy
                      << " drift_pct=" << stats.energyDriftPct
                      << (stats.energyEstimated ? " est" : "")
                      << " theta=" << ui.octreeTheta
                      << " soft=" << ui.octreeSoftening
                      << " sph=" << (ui.sphEnabled ? "on" : "off")
                      << " dt=" << stats.dt
                      << "\n";
            lastConsoleTrace = now;
        }

        if (now - lastTitleUpdate >= std::chrono::milliseconds(200)) {
            window.setTitle(buildWindowTitle(stats, ui, uiFps));
            lastTitleUpdate = now;
        }
    }

    backend.stop();
    return 0;
}

