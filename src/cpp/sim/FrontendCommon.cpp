#include "sim/FrontendCommon.hpp"

#include "sim/EnvUtils.hpp"
#include "sim/PlatformPaths.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace sim::frontend {
namespace {

std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

} // namespace

std::uint32_t resolveBackendParticleCount(const SimulationConfig &config)
{
    const std::uint32_t baseCount = std::max<std::uint32_t>(2u, config.particleCount);
    std::uint32_t requested = baseCount;

    std::uint32_t parsed = 0;
    if (sim::env::getNumber("GRAVITY_BACKEND_PARTICLES", parsed) && parsed > 1u) {
        requested = parsed;
    }
    return requested;
}

std::uint32_t resolveFrontendDrawCap(const SimulationConfig &config)
{
    const std::uint32_t cap = std::max<std::uint32_t>(2u, config.frontendParticleCap);
    std::uint32_t parsed = 0;
    if (sim::env::getNumber("GRAVITY_FRONTEND_DRAW_CAP", parsed) && parsed > 1u) {
        return parsed;
    }
    return cap;
}

std::string normalizeExportFormat(std::string_view raw)
{
    const std::string format = toLower(std::string(raw));
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

std::string extensionForExportFormat(std::string_view rawFormat)
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

std::string buildSuggestedExportPath(
    const std::string &directory,
    std::string_view format,
    std::uint64_t step)
{
    const std::string normalizedFormat = normalizeExportFormat(format.empty() ? std::string_view("vtk") : format);
    const std::string extension = extensionForExportFormat(normalizedFormat);

    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    const std::tm tm = sim::platform::localTime(nowTime);

    std::ostringstream fileName;
    fileName << "sim_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_s" << step << extension;
    const std::filesystem::path baseDir = directory.empty() ? std::filesystem::path("exports") : std::filesystem::path(directory);
    return (baseDir / fileName.str()).string();
}

} // namespace sim::frontend
