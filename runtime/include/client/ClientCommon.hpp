/*
 * @file runtime/include/client/ClientCommon.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTCOMMON_HPP_
#define BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTCOMMON_HPP_
#include <cstdint>
#include <string>
#include <string_view>
/*
 * @brief Defines the simulation config type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct SimulationConfig;

namespace bltzr_client {
std::uint32_t resolveServerParticleCount(const SimulationConfig& config);
std::uint32_t resolveClientDrawCap(const SimulationConfig& config);
std::string normalizeExportFormat(std::string_view raw);
std::string extensionForExportFormat(std::string_view rawFormat);
std::string inferExportFormatFromPath(const std::string& path);
std::string buildSuggestedExportPath(const std::string& directory, std::string_view format,
                                     std::uint64_t step);
} // namespace bltzr_client
#endif // BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTCOMMON_HPP_
