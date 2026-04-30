/*
 * @file engine/include/platform/PlatformPaths.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Platform abstraction interfaces for portable runtime services.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_PLATFORM_PLATFORMPATHS_HPP_
#define BLITZAR_ENGINE_INCLUDE_PLATFORM_PLATFORMPATHS_HPP_
#include <ctime>
#include <string>
#include <string_view>
#include <vector>

namespace bltzr_platform {
std::string executableName(std::string_view basename);
std::string_view serverDefaultExecutableName();
std::vector<std::string> sharedLibraryCandidates(std::string_view stem);
std::tm localTime(std::time_t nowTime);
} // namespace bltzr_platform
#endif // BLITZAR_ENGINE_INCLUDE_PLATFORM_PLATFORMPATHS_HPP_
