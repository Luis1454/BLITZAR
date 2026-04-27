// File: runtime/include/client/ClientCommon.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTCOMMON_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTCOMMON_HPP_
#include <cstdint>
#include <string>
#include <string_view>
/// Description: Defines the SimulationConfig data or behavior contract.
struct SimulationConfig;
namespace grav_client {
/// Description: Executes the resolveServerParticleCount operation.
std::uint32_t resolveServerParticleCount(const SimulationConfig& config);
/// Description: Executes the resolveClientDrawCap operation.
std::uint32_t resolveClientDrawCap(const SimulationConfig& config);
/// Description: Executes the normalizeExportFormat operation.
std::string normalizeExportFormat(std::string_view raw);
/// Description: Executes the extensionForExportFormat operation.
std::string extensionForExportFormat(std::string_view rawFormat);
/// Description: Executes the inferExportFormatFromPath operation.
std::string inferExportFormatFromPath(const std::string& path);
std::string buildSuggestedExportPath(const std::string& directory, std::string_view format,
                                     std::uint64_t step);
} // namespace grav_client
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTCOMMON_HPP_
