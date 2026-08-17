/*
 * @file engine/server/simulation/export/SrvExportInternal.hpp
 * @brief Internal snapshot export contracts.
 */

#ifndef BLITZAR_ENGINE_SERVER_SIMULATION_EXPORT_SRV_EXPORT_INTERNAL_HPP_
#define BLITZAR_ENGINE_SERVER_SIMULATION_EXPORT_SRV_EXPORT_INTERNAL_HPP_

#include "server/simulation/runtime/SrvRuntimeBase.hpp"

std::string defaultExportPath(const std::string& directory, const std::string& format,
                              std::uint64_t step);
std::string guessFormatFromPath(const std::string& path);

#endif
