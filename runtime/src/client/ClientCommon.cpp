// File: runtime/src/client/ClientCommon.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "client/ClientCommon.hpp"
#include "config/EnvUtils.hpp"
#include "config/SimulationConfig.hpp"
#include "config/SimulationOptionRegistry.hpp"
#include "platform/PlatformPaths.hpp"
#include "protocol/ServerProtocol.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
namespace grav_client {
/// Description: Executes the applyEnvOverride operation.
static void applyEnvOverride(std::string_view name, SimulationConfig& config)
{
    const std::optional<std::string> value = grav_env::get(name);
    if (!value.has_value()) {
        return;
    }
    (void)grav_config::applyEnvOption(std::string(name), *value, config, std::cerr);
}
/// Description: Executes the clampClientDrawCap operation.
static std::uint32_t clampClientDrawCap(std::uint32_t requested)
{
    if (requested > grav_protocol::kSnapshotMaxPoints)
        return grav_protocol::kSnapshotMaxPoints;
    return requested < 2u ? 2u : requested;
}
/// Description: Executes the toLower operation.
std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
/// Description: Executes the resolveServerParticleCount operation.
std::uint32_t resolveServerParticleCount(const SimulationConfig& config)
{
    SimulationConfig effective = config;
    /// Description: Executes the applyEnvOverride operation.
    applyEnvOverride("GRAVITY_SERVER_PARTICLES", effective);
    return std::max<std::uint32_t>(2u, effective.particleCount);
}
/// Description: Executes the resolveClientDrawCap operation.
std::uint32_t resolveClientDrawCap(const SimulationConfig& config)
{
    SimulationConfig effective = config;
    /// Description: Executes the applyEnvOverride operation.
    applyEnvOverride("GRAVITY_CLIENT_DRAW_CAP", effective);
    return clampClientDrawCap(effective.clientParticleCap);
}
/// Description: Executes the normalizeExportFormat operation.
std::string normalizeExportFormat(std::string_view raw)
{
    const std::string format = toLower(std::string(raw));
    if (format == "binary")
        return "bin";
    if (format == "vtkb" || format == "vtk-bin")
        return "vtk_binary";
    if (format == "vtk_ascii")
        return "vtk";
    return format;
}
/// Description: Executes the extensionForExportFormat operation.
std::string extensionForExportFormat(std::string_view rawFormat)
{
    const std::string format = normalizeExportFormat(rawFormat);
    if (format == "vtk" || format == "vtk_binary")
        return ".vtk";
    if (format == "xyz")
        return ".xyz";
    if (format == "bin")
        return ".bin";
    return {};
}
/// Description: Executes the inferExportFormatFromPath operation.
std::string inferExportFormatFromPath(const std::string& path)
{
    std::string ext = toLower(std::filesystem::path(path).extension().string());
    if (!ext.empty() && ext.front() == '.') {
        ext.erase(ext.begin());
    }
    if (ext == "vtk")
        return "vtk";
    if (ext == "xyz")
        return "xyz";
    if (ext == "bin" || ext == "binary" || ext == "nbin")
        return "bin";
    return {};
}
std::string buildSuggestedExportPath(const std::string& directory, std::string_view format,
                                     std::uint64_t step)
{
    const std::string normalizedFormat =
        /// Description: Executes the normalizeExportFormat operation.
        normalizeExportFormat(format.empty() ? std::string_view("vtk") : format);
    const std::string extension = extensionForExportFormat(normalizedFormat);
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    const std::tm tm = grav_platform::localTime(nowTime);
    std::ostringstream fileName;
    fileName << "sim_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_s" << step << extension;
    const std::filesystem::path baseDir =
        directory.empty() ? std::filesystem::path("exports") : std::filesystem::path(directory);
    return (baseDir / fileName.str()).string();
}
} // namespace grav_client
