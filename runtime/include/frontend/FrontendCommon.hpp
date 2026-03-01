#ifndef GRAVITY_SIM_FRONTENDCOMMON_HPP
#define GRAVITY_SIM_FRONTENDCOMMON_HPP

#include "config/SimulationConfig.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace grav_frontend {

std::uint32_t resolveBackendParticleCount(const SimulationConfig &config);

std::uint32_t resolveFrontendDrawCap(const SimulationConfig &config);

std::string normalizeExportFormat(std::string_view raw);
std::string extensionForExportFormat(std::string_view rawFormat);
std::string inferExportFormatFromPath(const std::string &path);
std::string buildSuggestedExportPath(
    const std::string &directory,
    std::string_view format,
    std::uint64_t step
);

} // namespace grav_frontend

#endif // GRAVITY_SIM_FRONTENDCOMMON_HPP

