#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTCOMMON_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTCOMMON_HPP_
#include <cstdint>
#include <string>
#include <string_view>
struct SimulationConfig;
namespace grav_client {
std::uint32_t resolveServerParticleCount(const SimulationConfig& config);
std::uint32_t resolveClientDrawCap(const SimulationConfig& config);
std::string normalizeExportFormat(std::string_view raw);
std::string extensionForExportFormat(std::string_view rawFormat);
std::string inferExportFormatFromPath(const std::string& path);
std::string buildSuggestedExportPath(const std::string& directory, std::string_view format,
                                     std::uint64_t step);
} // namespace grav_client
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTCOMMON_HPP_
