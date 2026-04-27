// File: engine/include/platform/PlatformPaths.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_PLATFORM_PLATFORMPATHS_HPP_
#define GRAVITY_ENGINE_INCLUDE_PLATFORM_PLATFORMPATHS_HPP_
#include <ctime>
#include <string>
#include <string_view>
#include <vector>

namespace grav_platform {
/// Description: Executes the executableName operation.
std::string executableName(std::string_view basename);
/// Description: Executes the serverDefaultExecutableName operation.
std::string_view serverDefaultExecutableName();
/// Description: Executes the sharedLibraryCandidates operation.
std::vector<std::string> sharedLibraryCandidates(std::string_view stem);
/// Description: Executes the localTime operation.
std::tm localTime(std::time_t nowTime);
} // namespace grav_platform
#endif // GRAVITY_ENGINE_INCLUDE_PLATFORM_PLATFORMPATHS_HPP_
